#include "browser_manager.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QQmlEngine>
#include <QUrl>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "Aria2CManager/engine_def.h"
#include "Browser/download_task_utils.h"
#include "Browser/download_url_utils.h"
#include "Browser/local_content_removal.h"
#include "Browser/stopped_task_delete_utils.h"
#include "Definitions/appDef.h"
#include "Parser/file_parser.h"
#include "PluginManager/plugin_manager.h"
#include "Settings/settings_manager.h"
#include "ed2k_engine_def.h"
#include "logger.h"
#include "os/os.h"
#include "toast/toast_manager.h"
#include "utils/utils.h"
namespace gdl {
	namespace ui {
		namespace browser {
			const static std::string kAria2OnDownloadStart		= "aria2.onDownloadStart";
			const static std::string kAria2OnDownloadPause		= "aria2.onDownloadPause";
			const static std::string kAria2OnDownloadStop		= "aria2.onDownloadStop";
			const static std::string kAria2OnDownloadComplete	= "aria2.onDownloadComplete";
			const static std::string kAria2OnDownloadError		= "aria2.onDownloadError";
			const static std::string kAria2onBtDownloadComplete = "aria2.onBtDownloadComplete";
			static const std::vector<std::string> keys			= {
				 "status", "totalLength", "completedLength", "downloadSpeed", "infoHash", "numSeeders",
				 "seeder", "connections", "errorCode",		 "errorMessage",  "dir",	  "files",
				 "gid",	   "bittorrent"};

			namespace {
				std::string Aria2HttpRpcHost() {
					const int port_value = settings::Settings::Instance().GetRpcListenPort();
					const std::string rpc_port =
						(port_value < 1024 || port_value > 65535) ? kEngineRpcPort : std::to_string(port_value);
					return std::string("http://127.0.0.1:") + rpc_port;
				}

				StoppedTaskAria2CleanupStatus RemoveAria2DownloadResultByGid(
					const QString& gid, QString* error_message) {
					engine::Aria2cHttpClient client(Aria2HttpRpcHost());
					auto http_result = client.RemoveDownloadResult(gid.toStdString());
					if (http_result.HasError()) {
						if (error_message) {
							const QString detail = QString::fromUtf8(http_result.GetError().what()).trimmed();
							*error_message =
								detail.isEmpty() ? QStringLiteral("aria2 RPC request failed") : detail;
						}
						return StoppedTaskAria2CleanupStatus::kFailed;
					}

					if (auto res = std::get_if<engine::ErrorResult>(&http_result.Value().result)) {
						const QString rpc_error = ExtractAria2RpcErrorMessage(res->body);
						if (IsMissingAria2ResultError(rpc_error)) {
							return StoppedTaskAria2CleanupStatus::kAlreadyMissing;
						}
						if (error_message) {
							const QString detail = rpc_error.isEmpty()
								? QString::fromStdString(res->err_msg).trimmed()
								: rpc_error;
							*error_message = detail.isEmpty()
								? QStringLiteral("aria2 RPC returned HTTP %1").arg(res->err_code)
								: detail;
						}
						return StoppedTaskAria2CleanupStatus::kFailed;
					}

					if (auto res = std::get_if<engine::SucceedResult>(&http_result.Value().result)) {
						const QString rpc_error = ExtractAria2RpcErrorMessage(res->body);
						if (IsMissingAria2ResultError(rpc_error)) {
							return StoppedTaskAria2CleanupStatus::kAlreadyMissing;
						}
						if (!rpc_error.isEmpty()) {
							if (error_message) {
								*error_message = rpc_error;
							}
							return StoppedTaskAria2CleanupStatus::kFailed;
						}
					}
					return StoppedTaskAria2CleanupStatus::kSucceeded;
				}

				// 从单个 aria2 任务对象解析出 DownloadTaskInfo,统一 OnHandleAria2Message 与
				// Aria2QueryByGidTaskInfo 两处重复逻辑。task_id 为空表示对象无有效路径(应跳过)。
				DownloadTaskInfo ParseAria2TaskObject(const nlohmann::json& object) {
					return DownloadTaskInfoFromAria2Object(object);
				}

				// ed2k 引擎 state 字符串 -> 现有 TaskState 枚举映射(与 aria2 任务共用同一套下载列表/历史管道)
				TaskState Ed2kStateStringToTaskState(const std::string& state) {
					if (state == "downloading" || state == "connecting") return TaskState::kActive;
					if (state == "paused") return TaskState::kPause;
					if (state == "completed") return TaskState::kComplete;
					if (state == "failed") return TaskState::kError;
					if (state == "cancelled") return TaskState::kRemoved;
					if (state == "queued") return TaskState::kWaiting;
					LOG_WARN("Unknown ed2k task state: {}", state);
					return TaskState::kActive;
				}

				// 非终态任务携带的 error 是"当前为什么还没进展"的状态说明,不是失败原因。引擎给的是
				// 面向开发者的英文短语,直接摊给用户既难懂也吓人(旧版把 "both client and all sources
				// are low id" 当失败原因红字显示,用户无从判断该等还是该放弃)。这里翻成可执行的说明。
				// 只对非失败态生效,失败任务仍原样展示引擎错误,避免掩盖真实故障。
				QString Ed2kWaitingReasonToHint(const QString& engine_message) {
					if (engine_message.contains(QStringLiteral("low id"), Qt::CaseInsensitive)) {
						return QObject::tr("Waiting for a connectable source: this client and all known sources are "
										   "behind NAT (LowID), so no direct connection is possible yet.");
					}
					if (engine_message.contains(QStringLiteral("file not found"), Qt::CaseInsensitive)) {
						return QObject::tr("Waiting for sources: no peer currently reports having this file.");
					}
					return engine_message;
				}

				// 从 ed2k 任务 ID(格式 "ed2k-<md4hex>")中提取 md4 十六进制串;非法格式返回空串
				QString Ed2kTaskIdToMd4(const QString& task_id) {
					static const QString kPrefix = QStringLiteral("ed2k-");
					if (!task_id.startsWith(kPrefix)) return {};
					return task_id.mid(kPrefix.size());
				}

				// 依据任务信息反向拼回 ed2k://|file|name|size|md4|/ 链接,供失败任务重试
				// (RetryTask -> AddHttpTask 会按 scheme 再次路由回 ed2k 引擎)。信息不全时返回空串。
				QString BuildEd2kDownloadLink(const QString& task_id, const QString& file_name,
											 std::int64_t total_size) {
					const QString md4 = Ed2kTaskIdToMd4(task_id);
					if (md4.isEmpty() || file_name.isEmpty() || total_size <= 0) return {};
					const QString encoded_name = QString::fromUtf8(QUrl::toPercentEncoding(file_name));
					return QStringLiteral("ed2k://|file|%1|%2|%3|/").arg(encoded_name).arg(total_size).arg(md4);
				}
			}  // namespace

			BrowserManager* BrowserManagerImpl::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
				Q_UNUSED(qmlengine)
				Q_UNUSED(jsengine);
				return &BrowserManagerImpl::Instance();
			}

			BrowserManagerImpl::~BrowserManagerImpl() {}

			void BrowserManagerImpl::SetEngineUnavailable(const QString& message) {
				if (!engine_available_ && engine_unavailable_message_ == message) return;
				engine_available_ = false;
				engine_unavailable_message_ = message;
				emit engineAvailabilityChanged();
				emit sigErrorMessage(message);
			}

			void BrowserManagerImpl::TriggerActivateWindow() {
				// 由主实例在检测到后续实例启动时调用，转成信号交 QML 侧提升主窗口
				emit sigActivateWindow();
			}

			Q_INVOKABLE void BrowserManagerImpl::SyncTrackersServerlist(){
				engine::Aria2cDownloadManager::Instance().UpdateMagnetServerList();
			}

			DownloadTaskModel* BrowserManagerImpl::GetActiveDownloadModel() {
				return active_model_.get();
			}

			DownloadTaskModel* BrowserManagerImpl::GetStopedDownloadModel() {
				return stopped_model_.get();
			}

			DownloadTaskModel* BrowserManagerImpl::GetWaitingDownloadModel() {
				return waiting_model_.get();
			}

			bool BrowserManagerImpl::AddHttpTask(const QVariantList& urls, const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value();
					if (value.canConvert<QStringList>()) {
						auto value_list = value.toStringList();
						for (const auto& value_item : value_list) {
							opt.emplace(key.toStdString(), value_item.toStdString());
						}
					}
					else if (value.canConvert<QString>()) {
						opt.emplace(key.toStdString(), value.toString().toStdString());
					}
				}
				int count = 0;
				for (const auto& url : urls) {
					if (url.canConvert<QString>()) {
						const QString url_str = url.toString().trimmed();
						// ed2k 链接走独立引擎,不进入 aria2 的 URL 规范化/添加流程
						if (IsEd2kLink(url_str)) {
							QString save_dir = options.value(QStringLiteral("dir")).toString().trimmed();
							if (save_dir.isEmpty()) {
								save_dir = settings::Settings::Instance().GetDir();
							}
							// 归一化:解码并安全校验文件名后重建链接,避免百分号编码名直接落盘(见 AddEd2kTask)
							const QString canonical_url = CanonicalizeEd2kLink(url_str);
							if (canonical_url.isEmpty()) {
								LOG_WARN("Rejected invalid ed2k link: {}", url_str.toStdString());
								Q_EMIT sigErrorMessage(tr("Invalid ed2k link: %1").arg(url_str));
								continue;
							}
							const auto id = engine::Ed2kDownloadManager::Instance().AddEd2kTask(
								canonical_url.toStdString(), save_dir.toStdString());
							if (id.empty()) {
								LOG_WARN("Failed to add ed2k download task: {}", url_str.toStdString());
								Q_EMIT sigErrorMessage(tr("Invalid ed2k link: %1").arg(url_str));
							}
							else {
								++count;
							}
							continue;
						}
						const auto normalized_url = NormalizeDownloadUrlForAria2(url_str);
						if (!normalized_url.has_value()) {
							LOG_WARN("Skip invalid download URL: {}", url_str.toStdString());
							Q_EMIT sigErrorMessage(tr("Invalid download link: %1").arg(url_str));
							continue;
						}
						auto task_options = opt;
						AddSuggestedOutOptionForUrl(task_options, *normalized_url);
						auto res = engine::Aria2cDownloadManager::Instance().AddHttpTask(
							{normalized_url->toStdString()}, task_options);
						if (res.HasError()) {
							LOG_ERR("Failed to add HTTP download task Download address {} error {}",
									url.toString().toStdString(), res.GetError().what());
							const QString detail = QString::fromUtf8(res.GetError().what()).trimmed();
							Q_EMIT sigErrorMessage(
								detail.isEmpty()
									? tr("Failed to add download task. Please check the link or aria2 connection.")
									: tr("Failed to add download task: %1").arg(detail));
							continue;
						}
						count++;
					}
				}
				return count > 0;
			}

			bool BrowserManagerImpl::AddTorrentTask(const QString& tarrent, const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value();
					if (value.canConvert<QStringList>()) {
						auto value_list = value.toStringList();
						for (const auto& value_item : value_list) {
							opt.emplace(key.toStdString(), value_item.toStdString());
						}
					}
					else if (value.canConvert<QString>()) {
						opt.emplace(key.toStdString(), value.toString().toStdString());
					}
				}
				if (!QFile::exists(tarrent)) {
					Q_EMIT sigErrorMessage(tr("Torrent file does not exist: %1").arg(tarrent));
					return false;
				}
				// 读取tarrent文件到base64
				QFile file(tarrent);
				if (!file.open(QIODevice::ReadOnly)) {
					Q_EMIT sigErrorMessage(tr("Failed to read torrent file: %1").arg(tarrent));
					return false;
				}
				QByteArray data = file.readAll();
				file.close();
				std::string base64_data = data.toBase64().toStdString();

				auto res = engine::Aria2cDownloadManager::Instance().AddTorrentTask(base64_data, opt);
				if (res.HasError()) {
					LOG_ERR("Failed to add Torrent download task Download address {} error {}", tarrent.toStdString(),
							res.GetError().what());
					const QString detail = QString::fromUtf8(res.GetError().what()).trimmed();
					Q_EMIT sigErrorMessage(
						detail.isEmpty()
							? tr("Failed to add torrent task. Please check the file or aria2 connection.")
							: tr("Failed to add torrent task: %1").arg(detail));
					return false;
				}

				return true;
			}

			bool BrowserManagerImpl::AddMetalinkTask(const QString& metalink, const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value();
					if (value.canConvert<QStringList>()) {
						auto value_list = value.toStringList();
						for (const auto& value_item : value_list) {
							opt.emplace(key.toStdString(), value_item.toStdString());
						}
					}
					else if (value.canConvert<QString>()) {
						opt.emplace(key.toStdString(), value.toString().toStdString());
					}
				}
				if (!QFile::exists(metalink)) {
					Q_EMIT sigErrorMessage(tr("Metalink file does not exist: %1").arg(metalink));
					return false;
				}
				// 读取metalink文件到base64
				QFile file(metalink);
				if (!file.open(QIODevice::ReadOnly)) {
					Q_EMIT sigErrorMessage(tr("Failed to read metalink file: %1").arg(metalink));
					return false;
				}
				QByteArray data = file.readAll();
				file.close();
				std::string base64_data = data.toBase64().toStdString();

				auto res = engine::Aria2cDownloadManager::Instance().AddMetalinkTask(base64_data, opt);
				if (res.HasError()) {
					LOG_ERR("Failed to add metalink download task Download address {} error {}", metalink.toStdString(),
							res.GetError().what());
					const QString detail = QString::fromUtf8(res.GetError().what()).trimmed();
					Q_EMIT sigErrorMessage(
						detail.isEmpty()
							? tr("Failed to add metalink task. Please check the file or aria2 connection.")
							: tr("Failed to add metalink task: %1").arg(detail));
					return false;
				}

				return true;
			}

			bool BrowserManagerImpl::PauseTask(int page_index, const QString& gid) {
				if (gid.isEmpty()) return false;
				if (gid.startsWith(QStringLiteral("ed2k-"))) {
					return engine::Ed2kDownloadManager::Instance().PauseTask(gid.toStdString());
				}
				if (page_index == 0) {

					return engine::Aria2cDownloadManager::Instance()
						.CallAria2cMethod(engine::Aria2Method::kPause, gid.toStdString())
						.IsOk();
				}
				else if (page_index == 1) {

					return engine::Aria2cDownloadManager::Instance()
						.CallAria2cMethod(engine::Aria2Method::kPause, gid.toStdString())
						.IsOk();
				}
				else {
					LOG_ERR("PauseTask error: page_index is invalid");
				}
				return false;
			}

			bool BrowserManagerImpl::PauseAllTask(int page_index) {
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							PauseTask(page_index, task);
						}
						return true;
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							PauseTask(page_index, task);
						}
						return true;
					}
				}
				else {
					LOG_ERR("PauseAllTask error: page_index is invalid");
				}
				return false;
			}

			bool BrowserManagerImpl::ForcePauseTask(int page_index, const QString& gid) {
				if (gid.isEmpty()) return false;
				if (page_index == 0) {
					if (active_model_) {
						return engine::Aria2cDownloadManager::Instance()
							.CallAria2cMethod(engine::Aria2Method::kForcePause, gid.toStdString())
							.IsOk();
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						return engine::Aria2cDownloadManager::Instance()
							.CallAria2cMethod(engine::Aria2Method::kForcePause, gid.toStdString())
							.IsOk();
					}
				}
				else {
					LOG_ERR("ForcePauseTask error: page_index is invalid");
				}
				return false;
			}

			bool BrowserManagerImpl::ForcePauseAllTask() {
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kForcePauseAll)
					.IsOk();
			}

			bool BrowserManagerImpl::UnpauseTask(int page_index, const QString& gid) {
				if (gid.isEmpty()) return false;
				if (gid.startsWith(QStringLiteral("ed2k-"))) {
					return engine::Ed2kDownloadManager::Instance().UnpauseTask(gid.toStdString());
				}
				if (page_index == 0) {
					if (active_model_) {
						return engine::Aria2cDownloadManager::Instance()
							.CallAria2cMethod(engine::Aria2Method::kUnpause, gid.toStdString())
							.IsOk();
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						return engine::Aria2cDownloadManager::Instance()
							.CallAria2cMethod(engine::Aria2Method::kUnpause, gid.toStdString())
							.IsOk();
					}
				}
				else {
					LOG_ERR("UnpauseTask error: page_index is invalid");
				}
				return false;
			}

			bool BrowserManagerImpl::UnpauseAllTask(int page_index) {
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							UnpauseTask(page_index, task);
						}
						return true;
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							UnpauseTask(page_index, task);
						}
						return true;
					}
				}
				else {
					LOG_ERR("UnpauseAllTask error: page_index is invalid");
				}
				return false;
			}

			QVariantMap BrowserManagerImpl::RemoveTask(int page_index, const QString& gid, bool is_remove_file) {
				return RemoveTaskResult(page_index, gid, is_remove_file).ToVariantMap();
			}

			TaskDeletionResult BrowserManagerImpl::RemoveTaskResult(int page_index, const QString& gid,
															 bool is_remove_file, int removal_attempts) {
				TaskDeletionResult result{.content_requested = is_remove_file};
				if (gid.isEmpty()) return result;

				// ed2k 任务由独立引擎管理,删除(含可选文件删除)在其网络线程内异步完成,不复用 aria2 清理路径
				if (gid.startsWith(QStringLiteral("ed2k-"))) {
					const bool removed =
						engine::Ed2kDownloadManager::Instance().RemoveTask(gid.toStdString(), is_remove_file);
					if (!removed) return result;
					if (active_model_) {
						active_model_->RemoveTaskById(gid);
					}
					if (waiting_model_) {
						waiting_model_->RemoveTaskById(gid);
					}
					result.task_removed = true;
					// 无 aria2 参与,置位仅表示"引擎侧清理无遗留",满足上层统一的删除结果判定
					result.aria2_cleaned = true;
					return result;
				}

                if (page_index != 0 && page_index != 1) {
					return result;
				}

				// 获取任务信息以获取文件路径（在删除任务之前）
				QString save_path;
				QString cache_file_path;
				if (page_index == 0 && active_model_) {
					auto task_info = active_model_->GetTaskById(gid);
					if (task_info) {
						save_path = task_info->task_save_path();
						cache_file_path = save_path + ".aria2";
					}
				} else if (page_index == 1 && waiting_model_) {
					auto task_info = waiting_model_->GetTaskById(gid);
					if (task_info) {
						save_path = task_info->task_save_path();
						cache_file_path = save_path + ".aria2";
					}
				}

				// 调用 aria2 删除任务
				const bool removed = engine::Aria2cDownloadManager::Instance()
									 .CallAria2cMethod(engine::Aria2Method::kRemove, gid.toStdString())
									 .IsOk();
				if (!removed) {
					return result;
				}

				// kRemove 成功后任务已离开 active/waiting 队列，模型必须立即反映这一事实。
				if (active_model_) {
					active_model_->RemoveTaskById(gid);
				}
				if (waiting_model_) {
					waiting_model_->RemoveTaskById(gid);
				}
				result.task_removed = true;
				const bool result_removed = engine::Aria2cDownloadManager::Instance()
										.CallAria2cMethod(engine::Aria2Method::kRemoveDownloadResult,
														 gid.toStdString())
										.IsOk();
				if (!result_removed) {
					return result;
				}
				result.aria2_cleaned = true;

				// 如果需要删除文件
				if (is_remove_file && !save_path.isEmpty()) {
					result.content = RemoveLocalContent(save_path, removal_attempts);
					result.control_file = RemoveLocalContent(cache_file_path, removal_attempts);
				}

				return result;
			}

			// 批量删除禁用本地文件删除的重试退避:批量在 UI 线程同步执行,
			// 多个被占用文件叠加的退避 sleep 会让界面冻结数百毫秒以上
			constexpr int kBulkRemovalAttempts = 1;

			QVariantMap BrowserManagerImpl::RemoveAllTask(int page_index, bool is_remove_file) {
				BulkDeletionResult bulk;
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							bulk.Add(RemoveTaskResult(page_index, task, is_remove_file, kBulkRemovalAttempts));
						}
						return bulk.ToVariantMap();
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							bulk.Add(RemoveTaskResult(page_index, task, is_remove_file, kBulkRemovalAttempts));
						}
						return bulk.ToVariantMap();
					}
				}
				else if (page_index == 2) {
					return RemoveAllStopTask(is_remove_file);
				}
				else {
					LOG_ERR("RemoveAllTask error: page_index is invalid");
				}
				return bulk.ToVariantMap();
			}

			bool BrowserManagerImpl::ForceRemoveTask(const QString& gid) {
				if (gid.isEmpty()) return false;
				const auto res = engine::Aria2cDownloadManager::Instance()
									 .CallAria2cMethod(engine::Aria2Method::kForceRemove, gid.toStdString())
									 .IsOk();
				if (res) {
					engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
						engine::Aria2Method::kRemoveDownloadResult, gid.toStdString());
					if (active_model_) {
						active_model_->RemoveTaskById(gid);
					}
				}
				return res;
			}

			bool BrowserManagerImpl::RemoveDownloadResult(const QString& gid) {
				if (gid.isEmpty()) return false;
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kRemoveDownloadResult, gid.toStdString())
					.IsOk();
			}

			bool BrowserManagerImpl::PurgeDownloadResult() {
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kPurgeDownloadResult)
					.IsOk();
			}

			bool BrowserManagerImpl::ChangeOption(const QString& gid, const QVariantMap& options) {
				if (gid.isEmpty()) return false;
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value().toString();
					opt.emplace(key.toStdString(), value.toStdString());
				}
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kChangeOption, gid.toStdString(), opt)
					.IsOk();
			}

			bool BrowserManagerImpl::ChangeGlobalOption(const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value().toString();
					opt.emplace(key.toStdString(), value.toStdString());
				}
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption, opt)
					.IsOk();
			}

			void BrowserManagerImpl::OpenFileLocation(const QString& file_path) {
				QFileInfo fileInfo(file_path);
				if (!fileInfo.exists()) return;

				QStringList args;
				QString program;

				if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) {
					program = "explorer";
					args << "/select," << QDir::toNativeSeparators(fileInfo.filePath());
				}
				else if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::MacOS) {
					program = "open";
					args << "-R" << fileInfo.filePath();
				}
				else {
					program = "xdg-open";
					args << fileInfo.path();
				}

				QProcess::startDetached(program, args);
			}

			bool BrowserManagerImpl::RetryTask(const QString& gid) {
				if (gid.isEmpty()) {
					Q_EMIT sigErrorMessage(tr("Failed to retry task: missing task id."));
					return false;
				}
				if (!stopped_model_) {
					Q_EMIT sigErrorMessage(tr("Failed to retry task: stopped task list is not available."));
					return false;
				}

				const DownloadTaskInfo* task = stopped_model_->GetTaskById(gid);
				if (!task) {
					Q_EMIT sigErrorMessage(tr("Failed to retry task: task was not found."));
					return false;
				}
				if (task->task_state() != TaskState::kError) {
					Q_EMIT sigErrorMessage(tr("Only failed tasks can be retried."));
					return false;
				}

				const auto request = BuildRetryTaskRequest(*task);
				if (!request.has_value()) {
					Q_EMIT sigErrorMessage(tr("Failed to retry task: original download link is unavailable."));
					return false;
				}
				if (!AddHttpTask(request->urls, request->options)) {
					return false;
				}
				const auto history_remove_result =
					gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(gid.toStdString());
				const bool history_removed = history_remove_result.IsOk();
				if (history_remove_result.HasError()) {
					LOG_ERR("Failed to remove retry task from history cache gid:{} error:{}", gid.toStdString(),
							history_remove_result.GetError().Describe());
				}

				// ed2k 任务在 aria2 中不存在,跳过对其无意义的 RPC 清理调用
				if (!gid.startsWith(QStringLiteral("ed2k-"))) {
					QString aria2_error;
					if (RemoveAria2DownloadResultByGid(gid, &aria2_error) ==
						StoppedTaskAria2CleanupStatus::kFailed) {
						LOG_WARN("Retry started but failed to remove old aria2 result gid:{} error:{}",
								 gid.toStdString(), aria2_error.toStdString());
					}
				}
				if (!stopped_model_->RemoveTaskById(gid)) {
					LOG_WARN("Retry started but failed to remove old stopped task gid:{}", gid.toStdString());
					return true;
				}
				if (!history_removed) {
					Q_EMIT sigErrorMessage(
						tr("Retry started, but the old failed task could not be removed from history."));
				}
				return true;
			}

			QVariantMap BrowserManagerImpl::RemoveStopTask(const QString& gid, bool is_remove_file) {
				return RemoveStopTaskResult(gid, is_remove_file).ToVariantMap();
			}

			TaskDeletionResult BrowserManagerImpl::RemoveStopTaskResult(const QString& gid,
															 bool is_remove_file, int removal_attempts) {
				TaskDeletionResult result{.content_requested = is_remove_file};
				if (gid.isEmpty()) {
					LOG_ERR("RemoveStopTask failed: missing task id");
					return result;
				}
				if (!stopped_model_) {
					LOG_ERR("RemoveStopTask failed: stopped task list is not available");
					return result;
				}

				const auto task = stopped_model_->GetTaskById(gid);
				if (!task) {
					LOG_ERR("RemoveStopTask failed: task was not found gid:{}", gid.toStdString());
					return result;
				}

				// ed2k 任务与 aria2 无关:不能走 aria2 RPC 清理(对 "ed2k-" 前缀 GID aria2 返回
				// "Invalid GID",不匹配"结果已缺失"判定,会导致删除流程中断、条目和历史永远删不掉)。
				// 终态任务通常已被引擎自动移出任务表,这里再调一次引擎移除仅作防御(不存在时为空操作);
				// 文件清理由本层负责:数据文件直接写在目标路径,控制文件为 <save_path>.part.met
				if (gid.startsWith(QStringLiteral("ed2k-"))) {
					if (engine::Ed2kDownloadManager::Instance().EngineIsRunning()) {
						engine::Ed2kDownloadManager::Instance().RemoveTask(gid.toStdString(), false);
					}
					const QString ed2k_save_path = task->task_save_path();
					if (!stopped_model_->RemoveTaskById(gid)) {
						LOG_ERR("RemoveStopTask failed to remove stopped model entry gid:{}", gid.toStdString());
						return result;
					}
					result.task_removed = true;
					// 无 aria2 参与,置位仅表示"引擎侧清理无遗留",满足上层统一的删除结果判定
					result.aria2_cleaned = true;
					const auto ed2k_history_result =
						gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(gid.toStdString());
					result.history_cleaned = ed2k_history_result.IsOk();
					if (ed2k_history_result.HasError()) {
						LOG_ERR("RemoveStopTask failed to remove history record gid:{} error:{}",
								gid.toStdString(), ed2k_history_result.GetError().Describe());
					}
					if (is_remove_file) {
						result.content = RemoveLocalContent(ed2k_save_path, removal_attempts);
						if (result.content.status == LocalRemovalStatus::kFailed) {
							LOG_WARN("RemoveStopTask failed to remove downloaded content gid:{} error:{}",
									 gid.toStdString(), result.content.error.toStdString());
						}
						result.control_file =
							RemoveLocalContent(ed2k_save_path + QStringLiteral(".part.met"), removal_attempts);
						if (result.control_file.status == LocalRemovalStatus::kFailed) {
							LOG_WARN("RemoveStopTask failed to remove ed2k control file gid:{} error:{}",
									 gid.toStdString(), result.control_file.error.toStdString());
						}
					}
					return result;
				}

				QString aria2_error;
				const auto aria2_cleanup_status = RemoveAria2DownloadResultByGid(gid, &aria2_error);
				const auto deletion_decision =
					DecideStoppedTaskDeletionAfterAria2Cleanup(aria2_cleanup_status, aria2_error);
				result.aria2_cleaned = deletion_decision.aria2_cleaned;

				const QString save_path = task->task_save_path();
				const QString cache_file_path = save_path + ".aria2";
				const auto res = stopped_model_->RemoveTaskById(gid);
				if (!res) {
					LOG_ERR("RemoveStopTask failed to remove stopped model entry gid:{}", gid.toStdString());
					return result;
				}
				result.task_removed = true;

				const auto history_remove_result =
					gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(gid.toStdString());
				result.history_cleaned = history_remove_result.IsOk();
				if (history_remove_result.HasError()) {
					LOG_ERR("RemoveStopTask failed to remove history record gid:{} error:{}", gid.toStdString(),
							history_remove_result.GetError().Describe());
				}
				if (deletion_decision.show_cleanup_warning) {
					LOG_WARN("RemoveStopTask aria2 cleanup warning gid:{} warning:{}", gid.toStdString(),
							 deletion_decision.warning_message.toStdString());
				}
				if (is_remove_file) {
					result.content = RemoveLocalContent(save_path, removal_attempts);
					if (result.content.status == LocalRemovalStatus::kFailed) {
						LOG_WARN("RemoveStopTask failed to remove downloaded content gid:{} error:{}",
								 gid.toStdString(), result.content.error.toStdString());
					}
					result.control_file = RemoveLocalContent(cache_file_path, removal_attempts);
					if (result.control_file.status == LocalRemovalStatus::kFailed) {
						LOG_WARN("RemoveStopTask failed to remove aria2 control file gid:{} error:{}",
								 gid.toStdString(), result.control_file.error.toStdString());
					}
				}
				return result;
			}

			QVariantMap BrowserManagerImpl::RemoveStopTask(int index, bool is_remove_file) {
				if (!stopped_model_) {
					LOG_ERR("RemoveStopTask failed: stopped task list is not available");
					return TaskDeletionResult{.content_requested = is_remove_file}.ToVariantMap();
				}
				auto task = stopped_model_->GetTask(index);
				if (!task) {
					LOG_ERR("RemoveStopTask failed: task was not found at index:{}", index);
					return TaskDeletionResult{.content_requested = is_remove_file}.ToVariantMap();
				}
				return RemoveStopTask(task->task_id(), is_remove_file);
			}

			QVariantMap BrowserManagerImpl::RemoveAllStopTask(bool is_remove_file) {
				BulkDeletionResult bulk;
				if (stopped_model_) {
					auto tasks = stopped_model_->GetTaskIds();
					for (const auto& task : tasks) {
						bulk.Add(RemoveStopTaskResult(task, is_remove_file, kBulkRemovalAttempts));
					}
					return bulk.ToVariantMap();
				}
				LOG_ERR("RemoveAllStopTask failed: stopped task list is not available");
				return bulk.ToVariantMap();
			}

			void BrowserManagerImpl::RefreshTaskList(int page_index) {
				if (page_index == 0) {
					if (active_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellActive,
																				   keys);
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellWaiting,
																				   0, 100, keys);
					}
				}
				else if (page_index == 2) {
					if (stopped_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellStopped,
																				   0, 100, keys);
					}
				}
				else {
					LOG_ERR("RefreshTaskList error: page_index is invalid");
				}
			}

			parser::FilePreviewModel* BrowserManagerImpl::GetFilePreviewModel(const QString& file_path) {
				QFileInfo file_info(file_path);
				if (!file_info.exists()) return nullptr;
				const auto suffix = file_info.suffix().toLower();
				if (suffix == "torrent") {
					parser::TorrentParser torrent;
					if (!torrent.Parse(file_path)) {
						return nullptr;
					}
					parser::FilePreviewModel* file_preview_model = new parser::FilePreviewModel();
					auto file_list_info							 = torrent.GetTorrentInfo();
					QVector<parser::PreviewFileInfo> file_model_list;
					for (const auto& file : file_list_info.files) {
						parser::PreviewFileInfo info;
						info.file_name		= file.file_name;
						info.file_extension = QFileInfo(file.file_path).suffix();
						info.file_size		= parser::PreviewFileInfo::FormatFileSize(file.file_size);
						file_model_list.append(info);
					}
					file_preview_model->setFiles(file_model_list);
					return file_preview_model;
				}
				else if (suffix == "meta4" || suffix == "metalink") {
					parser::MetalinkParser metallink;
					if (!metallink.Parse(file_path)) {
						return nullptr;
					}
					parser::FilePreviewModel* file_preview_model = new parser::FilePreviewModel();
					auto file_list_info							 = metallink.GetMetalinkInfo();
					QVector<parser::PreviewFileInfo> file_model_list;
					for (const auto& file : file_list_info.files) {
						parser::PreviewFileInfo info;
						info.file_name		= file.file_name;
						info.file_extension = QFileInfo(file.file_path).suffix();
						info.file_size		= parser::PreviewFileInfo::FormatFileSize(file.file_size);
						file_model_list.append(info);
					}
					file_preview_model->setFiles(file_model_list);
					return file_preview_model;
				}
				return nullptr;
			}

			parser::FilePreviewModel* BrowserManagerImpl::ParseEd2kLinks(const QString& text) {
				const auto entries = gdl::ui::browser::ParseEd2kLinks(text);
				if (entries.isEmpty()) return nullptr;

				auto* file_preview_model = new parser::FilePreviewModel();
				QVector<parser::PreviewFileInfo> file_model_list;
				file_model_list.reserve(entries.size());
				for (const auto& entry : entries) {
					parser::PreviewFileInfo info;
					info.file_name		= entry.name;
					info.file_extension = QFileInfo(entry.name).suffix();
					info.file_size		= parser::PreviewFileInfo::FormatFileSize(entry.size);
					info.is_selected	= true;
					file_model_list.append(info);
				}
				file_preview_model->setFiles(file_model_list);
				return file_preview_model;
			}

			QStringList BrowserManagerImpl::GetValidEd2kLinks(const QString& text) {
				const auto entries = gdl::ui::browser::ParseEd2kLinks(text);
				QStringList links;
				links.reserve(entries.size());
				for (const auto& entry : entries) {
					links.append(entry.raw);
				}
				return links;
			}

			bool BrowserManagerImpl::AddEd2kTask(const QVariantList& links, const QVariantMap& options) {
				QString save_dir = options.value(QStringLiteral("dir")).toString().trimmed();
				if (save_dir.isEmpty()) {
					save_dir = settings::Settings::Instance().GetDir();
				}
				bool any_succeeded = false;
				for (const auto& link : links) {
					if (!link.canConvert<QString>()) continue;
					const QString link_str = link.toString().trimmed();
					if (link_str.isEmpty()) continue;
					// 交给引擎前先归一化:把文件名段的百分号编码解码并做安全校验,再用解码后的
					// 文件名重建链接,否则 "My%20File.mkv"/CJK 编码名会被引擎原样拼进保存路径落盘。
					const QString canonical_link = CanonicalizeEd2kLink(link_str);
					if (canonical_link.isEmpty()) {
						LOG_WARN("Rejected invalid ed2k link: {}", link_str.toStdString());
						Q_EMIT sigErrorMessage(tr("Invalid ed2k link: %1").arg(link_str));
						continue;
					}
					const auto id = engine::Ed2kDownloadManager::Instance().AddEd2kTask(
						canonical_link.toStdString(), save_dir.toStdString());
					if (id.empty()) {
						LOG_WARN("Failed to add ed2k download task: {}", link_str.toStdString());
						Q_EMIT sigErrorMessage(tr("Invalid ed2k link: %1").arg(link_str));
					}
					else {
						any_succeeded = true;
					}
				}
				return any_succeeded;
			}

			bool BrowserManagerImpl::Init() {
				// 下载历史读取需在 DownloadHistoryCache::Initialize(mainwindow.cxx:86) 之后,
				// 故从构造函数移到此处;测试模式不调用 Init(),Fake 路径不加载历史(U1)
				InitDownloadHistoryCache();
				// subscribe aria2 responce
				auto res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2Response, [this](const std::string& msg) { OnHandleAria2Message(msg); });
				if (res.HasError()) return false;
				aria2_responce_subcription_ = res.Value();
				// subscribe active progress
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2ActiveProgress, [this](const std::string& msg) { OnHandleAria2ActiveProgress(msg); });
				if (res.HasError()) { UnInit(); return false; }
				aria2_active_progress_subcription_ = res.Value();
				// subscribe sync server list
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2SyncMagnetServerList, [this](const std::string& msg) { 
						Q_EMIT sigUpdateSyncServerList(QString::fromStdString(msg));
					});
				if (res.HasError()) { UnInit(); return false; }
				aria2_sync_server_list_subcription_ = res.Value();
				// subscribe tracker update status
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2TrackerUpdateStatus, [this](const std::string& msg) { OnHandleTrackerUpdateStatus(msg); });
				if (res.HasError()) { UnInit(); return false; }
				aria2_tracker_update_status_subscription_ = res.Value();

				// ed2k 引擎为可选能力:初始化失败仅记录日志,不影响 aria2 主流程可用性
				engine::Ed2kDownloadManager::Ed2kEngineConfig ed2k_config;
				ed2k_config.data_dir = os::GetAppDataDir() + "/gdownload/ed2k";
				// 引擎数据目录(server.met/known.met/nodes.dat 落盘处)必须先存在:
				// 引擎自身不建目录,缺失时首次持久化会静默失败,重启后服务器列表/哈希缓存全部丢失
				if (!QDir().mkpath(QString::fromStdString(ed2k_config.data_dir))) {
					LOG_WARN("Failed to create ed2k data dir: {}", ed2k_config.data_dir);
				}
				// 从设置系统读取引擎配置(SessionConfig 类设置下次启动生效)
				auto& ed2k_settings = settings::Settings::Instance();
				ed2k_config.nickname = ed2k_settings.GetEd2kNickname().toStdString();
				ed2k_config.tcp_port = static_cast<std::uint16_t>(ed2k_settings.GetEd2kTcpPort());
				ed2k_config.udp_port = static_cast<std::uint16_t>(ed2k_settings.GetEd2kUdpPort());
				ed2k_config.enable_kad = ed2k_settings.GetEd2kEnableKad();
				ed2k_config.enable_obfuscation = ed2k_settings.GetEd2kEnableObfuscation();
				ed2k_config.max_concurrent_tasks = static_cast<std::size_t>(ed2k_settings.GetEd2kMaxConcurrentTasks());
				// 持久 UserHash:首次生成(随机 16 字节 + eMule 标记字节[5]=0x0E/[14]=0x6F)后存
				// data_dir/user_hash.dat,跨启动稳定。远端按 hash 记上传队列等待与 credit;
				// 同 IP 换 hash 会被对端封禁 2 小时,固定共享 hash 则全网互相顶替队列记录。
				{
					const QString hash_path = QString::fromStdString(ed2k_config.data_dir) + "/user_hash.dat";
					QFile hash_file(hash_path);
					QByteArray hex;
					if (hash_file.open(QIODevice::ReadOnly)) {
						hex = hash_file.readAll().trimmed();
						hash_file.close();
					}
					if (hex.size() != 32) {
						QByteArray raw(16, Qt::Uninitialized);
						QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(raw.data()), 4);
						raw[5] = static_cast<char>(0x0E);
						raw[14] = static_cast<char>(0x6F);
						hex = raw.toHex();
						if (hash_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
							hash_file.write(hex);
							hash_file.close();
						} else {
							LOG_WARN("Failed to persist ed2k user hash to {}", hash_path.toStdString());
						}
					}
					ed2k_config.user_hash_hex = hex.toStdString();
				}
				// Kad 引导节点:引擎用 data_dir/nodes.dat 做 Kad DHT 种子,但引擎不自建/不下载。
				// 自动同步开启(ed2k.auto-sync-sources,默认 true)时,每次启动都从设置的 URL 刷新一份,
				// 保持节点列表新鲜;关闭时退回旧逻辑,仅在文件缺失或为空(仅文件头,<100 字节)时补下载一次。
				// 下载放在 InitEd2kEngine 之前(引擎 init-once,只在启动时读一次 nodes.dat);失败仅告警不阻断主流程。
				{
					const QString nodes_path = QString::fromStdString(ed2k_config.data_dir) + "/nodes.dat";
					QFileInfo nodes_info(nodes_path);
					const bool auto_sync = ed2k_settings.GetEd2kAutoSyncSources();
					if (auto_sync || !nodes_info.exists() || nodes_info.size() < 100) {
						const std::string url = ed2k_settings.GetEd2kNodesDatUrl().toStdString();
						if (!url.empty()) {
							// 复用系统代理(与 aria2 server.met 下载同口径);无代理时不设 Proxies 避免空串失败
							auto system_proxy = os::GetSystemHTTPProxy();
							std::string proxy_str;
							if (system_proxy.has_value()) {
								proxy_str = "http://" + system_proxy.value().first + ":" +
								            std::to_string(system_proxy.value().second);
							}
							cpr::Response reply;
							if (proxy_str.empty()) {
								reply = cpr::Get(cpr::Url(url), cpr::Timeout(5000));
							} else {
								reply = cpr::Get(cpr::Url(url),
								                 cpr::Proxies({{"http", proxy_str}, {"https", proxy_str}}),
								                 cpr::Timeout(5000));
							}
							if (reply.status_code == 200 && reply.text.size() >= 100) {
								QFile f(nodes_path);
								if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
									f.write(reply.text.data(), static_cast<qint64>(reply.text.size()));
									f.close();
									LOG_INFO("Downloaded ed2k Kad nodes.dat: {} bytes", reply.text.size());
								} else {
									LOG_WARN("Failed to write nodes.dat to {}", nodes_path.toStdString());
								}
							} else {
								LOG_WARN("Failed to download nodes.dat (status {}, {} bytes) from {}",
										  reply.status_code, reply.text.size(), url);
							}
						}
					}
				}
				if (!engine::Ed2kDownloadManager::Instance().InitEd2kEngine(ed2k_config)) {
					LOG_ERR("Failed to init ed2k engine, data_dir:{}", ed2k_config.data_dir);
				}
				else {
					ed2k_active_progress_subscription_ = engine::Ed2kDownloadManager::Instance()
						.SubscriptionEd2kMessage(kEd2kActiveProgress,
							[this](const std::string& msg) { OnHandleEd2kActiveProgress(msg); });
					ed2k_task_state_subscription_ = engine::Ed2kDownloadManager::Instance()
						.SubscriptionEd2kMessage(kEd2kTaskState,
							[this](const std::string& msg) { OnHandleEd2kTaskState(msg); });
					// 重启续传:必须在引擎初始化成功、订阅就绪之后才能重建任务
					RestoreEd2kDownloadHistory();
					// 启动链:自动同步开启时先刷新 server.met(引擎运行时下载),拿到结果后再自动连接。
					// 全新安装本地 server.met 为空,直接连接只会在内建 fallback 服务器上逐个耗尽
					// 每服务器 30s 的超时(最坏约 4 分钟);先更新(秒级)再连接,首连即可用上新列表。
					// UpdateServerMet 除重入忽略外的所有路径(成功/失败/引擎未就绪)都保证发布结果,
					// 链条不会悬挂。
					if (ed2k_settings.GetEd2kAutoSyncSources()) {
						if (ed2k_settings.GetEd2kAutoConnect()) {
							// 订阅必须在触发更新之前建立,避免错过结果。回调跑在引擎网络线程,
							// ConnectServer 内部会 post 到网络线程,跨线程调用安全。
							// 一次性原子标志保证仅首个结果触发自动连接;订阅句柄保留到 UnInit
							// 统一释放,后续用户手动"从 URL 更新"的结果不会再次触发。
							ed2k_boot_auto_connect_pending_.store(true);
							ed2k_server_met_boot_subscription_ = engine::Ed2kDownloadManager::Instance()
								.SubscriptionEd2kMessage(kEd2kServerMetResult, [this](const std::string&) {
									if (ed2k_boot_auto_connect_pending_.exchange(false)) {
										engine::Ed2kDownloadManager::Instance().ConnectServer(std::string(), 0);
									}
								});
						}
						engine::Ed2kDownloadManager::Instance().UpdateServerMet(
							ed2k_settings.GetEd2kServerMetUrl().toStdString());
					}
					else if (ed2k_settings.GetEd2kAutoConnect()) {
						// 自动同步关闭:保持原行为,直接用本地列表自动连接
						engine::Ed2kDownloadManager::Instance().ConnectServer(std::string(), 0);
					}
				}
				return true;
			}

			void BrowserManagerImpl::UnInit() {
				if (aria2_responce_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(aria2_responce_subcription_);
					aria2_responce_subcription_.reset();
				}
				if (aria2_active_progress_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_active_progress_subcription_);
					aria2_active_progress_subcription_.reset();
				}
				if (aria2_sync_server_list_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_sync_server_list_subcription_);
					aria2_sync_server_list_subcription_.reset();
				}
				if (aria2_tracker_update_status_subscription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_tracker_update_status_subscription_);
					aria2_tracker_update_status_subscription_.reset();
				}
				if (ed2k_active_progress_subscription_) {
					engine::Ed2kDownloadManager::Instance().UnSubscribeEd2kMessage(ed2k_active_progress_subscription_);
					ed2k_active_progress_subscription_.reset();
				}
				if (ed2k_task_state_subscription_) {
					engine::Ed2kDownloadManager::Instance().UnSubscribeEd2kMessage(ed2k_task_state_subscription_);
					ed2k_task_state_subscription_.reset();
				}
				if (ed2k_server_met_boot_subscription_) {
					engine::Ed2kDownloadManager::Instance().UnSubscribeEd2kMessage(ed2k_server_met_boot_subscription_);
					ed2k_server_met_boot_subscription_.reset();
				}
				engine::Ed2kDownloadManager::Instance().ShutdownEngine();
			}

			gdl::cache::DownloadRecord BrowserManagerImpl::DownloadTaskInfoToRecord(const DownloadTaskInfo& info) {
				return DownloadRecordFromTaskInfo(info);
			}

			DownloadTaskInfo BrowserManagerImpl::DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record) {
				return DownloadTaskInfoFromRecord(record);
			}

			BrowserManagerImpl::BrowserManagerImpl(QObject* parent) : QObject(parent) {
				active_model_  = std::make_unique<DownloadTaskModel>();
				waiting_model_ = std::make_unique<DownloadTaskModel>();
				stopped_model_  = std::make_unique<DownloadTaskModel>();
				connect(
					this, &BrowserManagerImpl::sigUpdateTasksMessage, this,
					[this](const DownloadTaskInfo& task_info) {
						try {
							const auto task_id = task_info.task_id();
							switch (task_info.task_state()) {
								case TaskState::kActive: {
									// 同一 GID 已进入新的活动生命周期时，允许它之后重新写入终态历史。
									stopped_model_->ClearTombstone(task_id);
									if (active_model_->ContainsTask(task_id)) {
										active_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										active_model_->AddTask(task_info);
									}
									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->RemoveTaskById(task_id);
									}
								} break;
								case TaskState::kWaiting: {
									stopped_model_->ClearTombstone(task_id);
									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										waiting_model_->AddTask(task_info);
									}
									if (active_model_->ContainsTask(task_id)) {
										active_model_->RemoveTaskById(task_id);
									}
								} break;
								case TaskState::kPause: {
									stopped_model_->ClearTombstone(task_id);
									if (active_model_->ContainsTask(task_id)) {
										active_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										active_model_->AddTask(task_info);
									}

									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->RemoveTaskById(task_id);
									}
								} break;
								case TaskState::kComplete:
								case TaskState::kRemoved:
								case TaskState::kError: {
									// 删除历史条目和 aria2/ed2k 的终态通知可能跨线程交错。
									// 仅 stopped 模型的墓碑表示该终态历史已被显式移除；active/
									// waiting 模型的墓碑也会在正常状态迁移时产生，不能用于此判断。
									if (stopped_model_->IsTombstoned(task_id)) {
										return;
									}
									if (active_model_->ContainsTask(task_id)) {
										active_model_->RemoveTaskById(task_id);
									}
									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->RemoveTaskById(task_id);
									}
									if (stopped_model_->ContainsTask(task_id)) {
										stopped_model_->UpdateTaskById(task_id, task_info);
										gdl::cache::DownloadRecord record = DownloadTaskInfoToRecord(task_info);
										const auto update_result =
											gdl::cache::DownloadHistoryCache::Instance().UpdateRecord(record);
										if (update_result.HasError()) {
											LOG_ERR("Failed to UPDATE record to history cache {} error:{}", record.save_path,
													update_result.GetError().Describe());
										}
									}
									else {
										stopped_model_->AddTask(task_info);
										gdl::cache::DownloadRecord record = DownloadTaskInfoToRecord(task_info);
										const auto add_result =
											gdl::cache::DownloadHistoryCache::Instance().AddRecord(record);
										if (add_result.HasError()) {
											LOG_ERR("Failed to add record to history cache {} error:{}", record.save_path,
													add_result.GetError().Describe());
										}
									}

								} break;

								default:
									LOG_WARN("Unknown task state:{}", static_cast<int>(task_info.task_state()));
									break;
							}
						} catch (std::exception& e) {
							LOG_ERR("{}", e.what());
						} catch (...) {}
					},
					Qt::QueuedConnection);

				connect(
					this, &BrowserManagerImpl::sigUpdateActiveProgress, this,
					[this](double progress) {
#if defined(_WIN32)
                        if (qApp->allWindows().isEmpty()) return;
						auto nativeWindowHandle = reinterpret_cast<void*>(qApp->allWindows().first()->winId());
						utils::UtilsToolsManager::Instance().SetTaskbarProgress(progress, nativeWindowHandle);

#elif defined(__APPLE__)
						utils::UtilsToolsManager::Instance().SetTaskbarProgress(progress);
#endif
					},
					Qt::QueuedConnection);

                connect(
                    this, &BrowserManagerImpl::sigErrorMessage, this,
                    [this](const QString& message) { toast::ToastManager::Instance().ShowError(message); },
                    Qt::QueuedConnection);
				connect(
					this, &BrowserManagerImpl::sigUpdateSyncServerList, this,
					[this](const QString& list) { 
						utils::UtilsToolsManager::Instance().SetserverList(list);
					},Qt::QueuedConnection);
				
			}
			void BrowserManagerImpl::InitDownloadHistoryCache() const {
				const auto records_result = gdl::cache::DownloadHistoryCache::Instance().GetRecords();
				if (records_result.HasError()) {
					LOG_ERR("Failed to read download history cache: {}", records_result.GetError().Describe());
					return;
				}
				for (const auto& record : records_result.Value()) {
					DownloadTaskInfo info = DownloadRecordToTaskInfo(record);
					if (stopped_model_ && !stopped_model_->ContainsTask(info.task_id())) {
						if (ShouldRestoreHistoryTask(info, QFile::exists(info.task_save_path()))) {
							stopped_model_->AddTask(info);
						}
						else {
							const auto delete_result = gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(
								info.task_id().toStdString());
							if (delete_result.HasError()) {
								LOG_ERR("Failed to remove stale history record gid:{} error:{}",
										info.task_id().toStdString(), delete_result.GetError().Describe());
							}
						}
					}
				}
			}
			// 重启续传:ed2k 引擎在进程内运行,没有像 aria2c 那样依赖外部进程 --save-session/
			// --input-file 的会话续传机制,Session 重建后 id_to_session 映射为空、之前的任务需要
			// 显式重新调用 AddEd2kTask 才能被重新纳入调度——引擎内部按 md4 定位同名 .part.met
			// 实现真正的断点续传(不会从 0 重新下载已有分片)。
			// 只处理 kError:kRemoved 代表用户已主动移除任务,不应自动复活;kComplete 已完成无需处理;
			// kActive/kWaiting/kPause 这几个非终态从不写入历史缓存(参见 sigUpdateTasksMessage 中
			// 落库逻辑只在 kComplete/kRemoved/kError 分支触发),因此这里能观察到的"非 complete"
			// 记录事实上只有 kError 一种。
			// 调用时机:必须晚于 ed2k 引擎 InitEd2kEngine 成功(AddEd2kTask 依赖 Session 已在跑)。
			void BrowserManagerImpl::RestoreEd2kDownloadHistory() const {
				const auto records_result = gdl::cache::DownloadHistoryCache::Instance().GetRecords();
				if (records_result.HasError()) {
					// 错误已由 InitDownloadHistoryCache 记录过一次,这里不重复打日志
					return;
				}
				for (const auto& record : records_result.Value()) {
					if (record.state != gdl::cache::DownloadState::kError) continue;
					const QString link = QString::fromStdString(record.download_url);
					if (!IsEd2kLink(link)) continue;

					// save_path 存的是完整目标文件路径,反推所在目录作为 save_dir;取不到时退回默认下载目录
					QString save_dir = QFileInfo(QString::fromStdString(record.save_path)).path();
					if (save_dir.isEmpty() || save_dir == QStringLiteral(".")) {
						save_dir = settings::Settings::Instance().GetDir();
					}
					const auto task_id = engine::Ed2kDownloadManager::Instance().AddEd2kTask(
						record.download_url, save_dir.toStdString());
					if (task_id.empty()) {
						LOG_WARN("Failed to restore ed2k task on restart: {}", record.download_url);
						continue;
					}
					// 恢复成功:task_id 由链接的 md4 确定性生成,与旧记录的 task_id 相同;移除旧的失败态
					// 展示条目与历史记录,避免和即将到来的新状态事件重复——与 RetryTask 的既有约定一致。
					if (stopped_model_) {
						stopped_model_->RemoveTaskById(QString::fromStdString(task_id));
					}
					const auto delete_result = gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(task_id);
					if (delete_result.HasError()) {
						LOG_ERR("Failed to remove resumed ed2k record from history cache task_id:{} error:{}",
								task_id, delete_result.GetError().Describe());
					}
				}
			}

			void BrowserManagerImpl::OnHandleAria2Message(const std::string& msg) {
				try {

					nlohmann::json doc = nlohmann::json::parse(msg);
					if (doc.find("result") != doc.end()) {
						// succeed messgae
						auto result = doc["result"];
						if (result.is_array()) {
							// array
							if (!result.empty()) {
								// task result array
								for (const auto& object : result) {
									DownloadTaskInfo task_info = ParseAria2TaskObject(object);
									// task_id 为空表示 BT/元数据阶段无有效路径,跳过此任务
									if (task_info.task_id().isEmpty()) continue;
									Q_EMIT sigUpdateTasksMessage(task_info);
								}
								//LOG_DBG("OnHandleAria2Message  array {}", result.dump());
							}
						}
						else if (result.is_object()) {
							// object
							LOG_INFO("OnHandleAria2Message  object {}", result.dump());
						}
						else if (result.is_string()) {
							// string
							LOG_DBG("OnHandleAria2Message  string {}", result.dump());
						}
						else if (result.is_number()) {
							// number
							LOG_DBG("OnHandleAria2Message  number {}", result.dump());
						}
					}
					else if (doc.find("error") != doc.end()) {
						// error message
						auto error_object = doc["error"];
						LOG_ERR("OnHandleAria2Message  error {}", error_object.dump());
						if (error_object.is_object() && error_object.find("message") != error_object.end()) {
							QString error_message = QString::fromStdString(error_object["message"].get<std::string>());
							Q_EMIT sigErrorMessage(error_message);
						}
					}
					else if (doc.find("method") != doc.end()) {
						// method message
						LOG_DBG("OnHandleAria2Message  method {}", doc.dump());
						const auto method	 = doc["method"].get<std::string>();
						const auto params	 = doc["params"];
						auto get_param_gids = [](const nlohmann::json& param) {
							std::vector<std::string> gids;
							for (const auto& item : param) {
								if (item.contains("gid") && item["gid"].is_string()) {
									gids.push_back(item["gid"].get<std::string>());
								}
							}
							return gids;
						};
						auto get_params_task = [&get_param_gids](const nlohmann::json& param) {
							DownloadTaskInfo task_info;
							for (const auto& gid : get_param_gids(param)) {
								task_info = Aria2QueryByGidTaskInfo(gid);
							}
							return task_info;
						};
						auto emit_download_error = [this](const std::string& gid) {
							const QString detail = Aria2QueryByGidErrorMessage(gid);
							Q_EMIT sigErrorMessage(
								detail.isEmpty()
									? tr("Download failed. Please check the link or network connection.")
									: tr("Download failed: %1").arg(detail));
						};

						if (method == kAria2OnDownloadStart) {
							//aria2.onDownloadStart
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								return;
							}
							Q_EMIT sigUpdateTasksMessage(task);

							// 执行用户配置的开始后操作
							auto action = settings::Settings::Instance().GetOnStartAction();
							if (action == 1) {  // 播放声音
								PlayNotificationSound();
							}
						}
						else if (method == kAria2OnDownloadPause) {
							//aria2.onDownloadPause
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
									engine::Aria2Method::kTellActive, keys);
								return;
							}
							Q_EMIT sigUpdateTasksMessage(task);
						}
						else if (method == kAria2OnDownloadStop) {
							//aria2.onDownloadStop
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								return;
							}
							task.set_task_state(TaskState::kRemoved);
							Q_EMIT sigUpdateTasksMessage(task);
						}
						else if (method == kAria2OnDownloadComplete || method == kAria2onBtDownloadComplete) {
							// aria2.onDownloadComplete
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								return;
							}
							task.set_task_state(TaskState::kComplete);
							Q_EMIT sigUpdateTasksMessage(task);

							// 执行用户配置的完成后操作
							auto action = settings::Settings::Instance().GetOnCompleteAction();
							auto customCmd = settings::Settings::Instance().GetCustomCompleteCommand();
							ExecutePostDownloadAction(task, action, customCmd);
						}
						else if (method == kAria2OnDownloadError) {
							// aria2.onDownloadError
							const auto gids = get_param_gids(params);
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								if (gids.empty()) {
									Q_EMIT sigErrorMessage(
										tr("Download failed. Please check the link or network connection."));
								} else {
									for (const auto& gid : gids) {
										emit_download_error(gid);
									}
								}
								return;
							}
							task.set_task_state(TaskState::kError);
							Q_EMIT sigUpdateTasksMessage(task);
							if (!gids.empty()) {
								emit_download_error(gids.back());
							}

							// 执行用户配置的错误后操作
							auto action = settings::Settings::Instance().GetOnErrorAction();
							auto customCmd = settings::Settings::Instance().GetCustomErrorCommand();
							// 错误操作只支持播放声音和自定义命令
							if (action == 1) {  // 播放声音
								PlayNotificationSound();
							} else if (action == 2 && !customCmd.isEmpty()) {  // 自定义命令
								QString filePath = task.task_save_path();
								QString gid = task.task_id();
								QFileInfo fileInfo(filePath);
								QString dir = fileInfo.absolutePath();
								ExecuteCustomCommand(customCmd, filePath, dir, gid);
							}
						}
						else {
							LOG_WARN("Unknown method type: {}", method);
						}
					}
				} catch (std::exception& e) {
					LOG_ERR("{}", e.what());
				} catch (...) {
					LOG_ERR("OnHandleAria2Message exception");
				}
			}

			void BrowserManagerImpl::OnHandleAria2ActiveProgress(const std::string& msg) {
				try {
					nlohmann::json doc = nlohmann::json::parse(msg);
					if (doc.is_object()) {
						if (doc.contains("totalLength") && doc.contains("completedLength")) {
							std::int64_t total_length	  = doc["totalLength"].is_string() ? std::stoll(doc["totalLength"].get<std::string>()) : doc["totalLength"].get<std::int64_t>();
							std::int64_t completed_length = doc["completedLength"].is_string() ? std::stoll(doc["completedLength"].get<std::string>()) : doc["completedLength"].get<std::int64_t>();
							// 无活动任务(total==0)必须发负值清除任务栏/Dock 进度条;
							// 发 0.0 会被平台层当作"0% 进行中"而常驻空进度条
							double progress				  = -1.0;
							if (total_length > 0) {
								progress = completed_length * 1.0 / total_length;
							}
							Q_EMIT sigUpdateActiveProgress(progress);
						}
					}
				} catch (std::exception& e) {
					LOG_ERR("{}", e.what())
				} catch (...) {
					LOG_ERR("OnHandleAria2ActiveProgress exception");
				}
			}

			void BrowserManagerImpl::OnHandleTrackerUpdateStatus(const std::string& msg) {
				Q_EMIT sigTrackerUpdateStatus(QString::fromStdString(msg));
			}

			// 1s 采样的活动任务进度数组:[{id,name,total,done,speed,sources,state}, ...]
			// 与 OnHandleAria2ActiveProgress 一样在 ed2k 引擎网络线程回调,经 Qt::QueuedConnection
			// 的 sigUpdateTasksMessage 转发到主线程,构造函数里既有的 lambda 负责落 model + 历史。
			void BrowserManagerImpl::OnHandleEd2kActiveProgress(const std::string& msg) {
				try {
					const nlohmann::json doc = nlohmann::json::parse(msg);
					if (!doc.is_array()) return;
					for (const auto& item : doc) {
						if (!item.is_object() || !item.contains("id") || !item["id"].is_string()) continue;
						const QString task_id = QString::fromStdString(item["id"].get<std::string>());
						if (task_id.isEmpty()) continue;

						DownloadTaskInfo task_info;
						task_info.set_task_id(task_id);
						task_info.set_task_file_name(QString::fromStdString(item.value("name", std::string())));
						task_info.set_task_total_size(item.value("total", static_cast<std::int64_t>(0)));
						task_info.set_task_current_size(item.value("done", static_cast<std::int64_t>(0)));
						task_info.set_task_download_speed(item.value("speed", static_cast<std::int64_t>(0)));
						// "连接数"取 active_sources(此刻真正在连的源), 与 aria2 任务那侧 connections 的
						// 口径一致; sources(迄今发现的源总数, 含已放弃/冷却中的)另开一个标签展示 ——
						// 它才是判断服务器周期重问 / Kad 周期查源 / SX2 源交换有没有把源集合做大的依据。
						// 旧引擎不带 active_sources 时回退到 sources, 即改动前的显示口径。
						const auto known_sources = item.value("sources", static_cast<std::int64_t>(0));
						task_info.set_task_connections(item.value("active_sources", known_sources));
						task_info.set_task_sources(known_sources);
						task_info.set_task_download_link(
							BuildEd2kDownloadLink(task_id, task_info.task_file_name(), task_info.task_total_size()));
						task_info.set_task_save_path(QString::fromStdString(item.value("out_path", std::string())));
						const TaskState sampled_state =
							Ed2kStateStringToTaskState(item.value("state", std::string()));
						task_info.set_task_state(sampled_state);
						// 等待原因(非失败态的 error)翻成用户能据以决策的说明; 空字符串会清掉上一轮的提示
						const QString sampled_reason =
							QString::fromStdString(item.value("error", std::string()));
						task_info.set_task_error_message(
							sampled_reason.isEmpty() || sampled_state == TaskState::kError
								? sampled_reason
								: Ed2kWaitingReasonToHint(sampled_reason));

						// 缓存最近一次采样,供 OnHandleEd2kTaskState 在仅含 id/state/error 的终态事件中补全字段;
						// 两个回调同在 ed2k 网络线程单线程串行执行,读写此表无需加锁(详见头文件成员注释)。
						ed2k_task_cache_[task_id] = task_info;
						Q_EMIT sigUpdateTasksMessage(task_info);
					}
				} catch (const std::exception& e) {
					LOG_ERR("{}", e.what())
				} catch (...) {
					LOG_ERR("OnHandleEd2kActiveProgress exception");
				}
			}

			// 单任务状态变更事件:{id,state,error}。payload 不含文件名/大小,从采样缓存补全后再转发。
			void BrowserManagerImpl::OnHandleEd2kTaskState(const std::string& msg) {
				try {
					const nlohmann::json doc = nlohmann::json::parse(msg);
					if (!doc.is_object() || !doc.contains("id") || !doc["id"].is_string()) return;
					const QString task_id = QString::fromStdString(doc["id"].get<std::string>());
					if (task_id.isEmpty()) return;

					DownloadTaskInfo task_info = ed2k_task_cache_.value(task_id);
					task_info.set_task_id(task_id);
					const TaskState state = Ed2kStateStringToTaskState(doc.value("state", std::string()));
					task_info.set_task_state(state);
					const QString error = QString::fromStdString(doc.value("error", std::string()));
					if (!error.isEmpty()) {
						// 失败态: 原样展示引擎错误; 非失败态: 这是"还在等什么"的说明, 翻成友好文案
						task_info.set_task_error_message(state == TaskState::kError
															 ? error
															 : Ed2kWaitingReasonToHint(error));
					}
					if (task_info.task_download_link().isEmpty()) {
						task_info.set_task_download_link(BuildEd2kDownloadLink(
							task_id, task_info.task_file_name(), task_info.task_total_size()));
					}

					// 终态之后该任务不会再出现在采样数组中,清理缓存避免无界增长;非终态则刷新缓存内容
					if (state == TaskState::kComplete || state == TaskState::kError || state == TaskState::kRemoved) {
						ed2k_task_cache_.remove(task_id);
					}
					else {
						ed2k_task_cache_[task_id] = task_info;
					}
					Q_EMIT sigUpdateTasksMessage(task_info);
				} catch (const std::exception& e) {
					LOG_ERR("{}", e.what())
				} catch (...) {
					LOG_ERR("OnHandleEd2kTaskState exception");
				}
			}

			DownloadTaskInfo BrowserManagerImpl::Aria2QueryByGidTaskInfo(const std::string& gid) {
			DownloadTaskInfo task_info;
			// 获取并验证 RPC 端口
			int port_value = settings::Settings::Instance().GetRpcListenPort();
			std::string rpc_port;
			// 验证端口范围，如果无效则使用默认值
			if (port_value < 1024 || port_value > 65535) {
				rpc_port = kEngineRpcPort;
			} else {
				rpc_port = std::to_string(port_value);
			}
			const std::string host = std::string("http://127.0.0.1:") + rpc_port;
				engine::Aria2cHttpClient client(host);
				auto http_result = client.TellStatus(gid, keys);
				if (http_result.HasError()) {
					LOG_ERR("Failed to query task info by gid:{} error:{}", gid, http_result.GetError().what())
					return task_info;
				}
				if (auto res = std::get_if<engine::ErrorResult>(&http_result.Value().result)) {
					LOG_ERR("Failed to query task info by gid:{} error:{}", gid, res->err_msg)
					return task_info;
				}
				else if (auto res = std::get_if<engine::SucceedResult>(&http_result.Value().result)) {
					try {
						nlohmann::json doc = nlohmann::json::parse(res->body);
						if (doc.find("result") != doc.end()) {
							// succeed messgae
							auto object = doc["result"];
							if (object.is_object()) {
								task_info = ParseAria2TaskObject(object);
								if (task_info.task_id().isEmpty()) {
									LOG_WARN("Failed to get file path by gid:{}", gid)
								}
							}
						}
						else {
							LOG_INFO("TellStatus result parse fail {}", res->body);
						}
					} catch (std::exception& e) {
						LOG_ERR("{}", e.what())
					}
				}
				return task_info;
			}

			QString BrowserManagerImpl::Aria2QueryByGidErrorMessage(const std::string& gid) {
				// 获取并验证 RPC 端口
				int port_value = settings::Settings::Instance().GetRpcListenPort();
				std::string rpc_port;
				// 验证端口范围，如果无效则使用默认值
				if (port_value < 1024 || port_value > 65535) {
					rpc_port = kEngineRpcPort;
				} else {
					rpc_port = std::to_string(port_value);
				}
				const std::string host = std::string("http://127.0.0.1:") + rpc_port;
				engine::Aria2cHttpClient client(host);
				auto http_result = client.TellStatus(gid, keys);
				if (http_result.HasError()) {
					LOG_ERR("Failed to query task error by gid:{} error:{}", gid, http_result.GetError().what())
					return {};
				}
				if (auto res = std::get_if<engine::ErrorResult>(&http_result.Value().result)) {
					LOG_ERR("Failed to query task error by gid:{} error:{}", gid, res->err_msg)
					return QString::fromStdString(res->err_msg).trimmed();
				}
				else if (auto res = std::get_if<engine::SucceedResult>(&http_result.Value().result)) {
					try {
						nlohmann::json doc = nlohmann::json::parse(res->body);
						if (doc.find("result") != doc.end() && doc["result"].is_object()) {
							const auto& object = doc["result"];
							if (object.contains("errorMessage") && object["errorMessage"].is_string()) {
								return QString::fromStdString(object["errorMessage"].get<std::string>()).trimmed();
							}
						}
						else if (doc.find("error") != doc.end() && doc["error"].is_object()) {
							const auto& error = doc["error"];
							if (error.contains("message") && error["message"].is_string()) {
								return QString::fromStdString(error["message"].get<std::string>()).trimmed();
							}
						}
					} catch (std::exception& e) {
						LOG_ERR("{}", e.what())
					}
				}
				return {};
			}

			// 执行下载完成后的操作
			void BrowserManagerImpl::ExecutePostDownloadAction(
				const DownloadTaskInfo& task,
				int actionType,
				const QString& customCommand) {

				QString filePath = task.task_save_path();
				QString gid = task.task_id();
				QFileInfo fileInfo(filePath);
				QString dir = fileInfo.absolutePath();

				switch (actionType) {
					case 0:  // 无操作
						break;

					case 1:  // 打开文件
						#ifdef _WIN32
							QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(filePath)});
						#elif __APPLE__
							QProcess::startDetached("open", {filePath});
						#else
							QProcess::startDetached("xdg-open", {filePath});
						#endif
						break;

					case 2:  // 打开目录
						#ifdef _WIN32
							QProcess::startDetached("explorer", {QDir::toNativeSeparators(dir)});
						#elif __APPLE__
							QProcess::startDetached("open", {dir});
						#else
							QProcess::startDetached("xdg-open", {dir});
						#endif
						break;

					case 3:  // 播放声音
						PlayNotificationSound();
						break;

					case 4:  // 自定义命令
						if (!customCommand.isEmpty()) {
							ExecuteCustomCommand(customCommand, filePath, dir, gid);
						}
						break;

					case 5:  // 关机
						#ifdef _WIN32
							QProcess::startDetached("shutdown", {"/s", "/t", "60"});  // 60秒后关机
						#elif __APPLE__
							QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to shut down"});
						#else
							QProcess::startDetached("shutdown", {"-h", "+1"});
						#endif
						break;

					case 6:  // 睡眠
						#ifdef _WIN32
							QProcess::startDetached("rundll32.exe", {"powrprof.dll,SetSuspendState", "0,1,0"});
						#elif __APPLE__
							QProcess::startDetached("pmset", {"sleepnow"});
						#else
							QProcess::startDetached("systemctl", {"suspend"});
						#endif
						break;

					case 7:  // 重启
						#ifdef _WIN32
							QProcess::startDetached("shutdown", {"/r", "/t", "60"});
						#elif __APPLE__
							QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to restart"});
						#else
							QProcess::startDetached("shutdown", {"-r", "+1"});
						#endif
						break;

					default:
						LOG_WARN("Unknown post-download action type: {}", actionType);
						break;
				}
			}

			// 执行自定义命令（替换变量）
			void BrowserManagerImpl::ExecuteCustomCommand(
				const QString& command,
				const QString& filePath,
				const QString& dir,
				const QString& gid) {

				// 替换变量
				QString cmd = command;
				cmd.replace("{file}", filePath);
				cmd.replace("{dir}", dir);
				cmd.replace("{gid}", gid);

				// 异步执行命令
				LOG_INFO("Executing custom command: {}", cmd.toStdString());
				{ auto parts = QProcess::splitCommand(cmd); if (!parts.isEmpty()) QProcess::startDetached(parts.takeFirst(), parts); }
			}

			// 播放通知声音
			void BrowserManagerImpl::PlayNotificationSound() {
				#ifdef _WIN32
					// Windows 使用系统通知声音
					QProcess::startDetached("powershell", {"-Command", "(New-Object Media.SoundPlayer 'C:\\Windows\\Media\\Windows Notify.wav').PlaySync();"});
				#elif __APPLE__
					// macOS 使用系统声音
					QProcess::startDetached("afplay", {"/System/Library/Sounds/Glass.aiff"});
				#else
					// Linux 使用 PulseAudio
					QProcess::startDetached("paplay", {"/usr/share/sounds/freedesktop/stereo/complete.oga"});
				#endif
			}

			void RegisterTypes(QQmlEngine* engine) {
				Q_UNUSED(engine);
				// 单例注册已迁移至 MainWindow::InitQmlEngine,经由 createBrowserManager 工厂注入
				// aria2c 引擎初始化与 BrowserManagerImpl::Init() 迁移至 MainWindow,按测试模式跳过
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
