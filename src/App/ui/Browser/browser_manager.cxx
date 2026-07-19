#include "browser_manager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QQmlEngine>
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

				QString ExtractAria2RpcErrorMessage(const std::string& body) {
					try {
						const nlohmann::json doc = nlohmann::json::parse(body);
						if (doc.contains("error") && doc["error"].is_object()) {
							const auto& error = doc["error"];
							if (error.contains("message") && error["message"].is_string()) {
								return QString::fromStdString(error["message"].get<std::string>()).trimmed();
							}
						}
					} catch (const std::exception& e) {
						LOG_ERR("Failed to parse aria2 RPC response: {}", e.what());
					}
					return {};
				}

				bool IsMissingAria2ResultError(const QString& message) {
					const QString lower = message.toLower();
					return lower.contains(QStringLiteral("gid")) && lower.contains(QStringLiteral("not found"));
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
						if (error_message) {
							const QString detail = QString::fromStdString(res->err_msg).trimmed();
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
															 bool is_remove_file) {
				TaskDeletionResult result{.content_requested = is_remove_file};
				if (gid.isEmpty()) return result;
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
					result.content = RemoveLocalContent(save_path);
					result.control_file = RemoveLocalContent(cache_file_path);
				}

				return result;
			}

			QVariantMap BrowserManagerImpl::RemoveAllTask(int page_index, bool is_remove_file) {
				BulkDeletionResult bulk;
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							bulk.Add(RemoveTaskResult(page_index, task, is_remove_file));
						}
						return bulk.ToVariantMap();
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							bulk.Add(RemoveTaskResult(page_index, task, is_remove_file));
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

				QString aria2_error;
				if (RemoveAria2DownloadResultByGid(gid, &aria2_error) ==
					StoppedTaskAria2CleanupStatus::kFailed) {
					LOG_WARN("Retry started but failed to remove old aria2 result gid:{} error:{}",
							 gid.toStdString(), aria2_error.toStdString());
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
															 bool is_remove_file) {
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

				QString aria2_error;
				const auto aria2_cleanup_status = RemoveAria2DownloadResultByGid(gid, &aria2_error);
				const auto deletion_decision =
					DecideStoppedTaskDeletionAfterAria2Cleanup(aria2_cleanup_status, aria2_error);
				if (!deletion_decision.remove_local_task) {
					LOG_WARN("RemoveStopTask failed during aria2 cleanup gid:{} error:{}", gid.toStdString(),
							 aria2_error.toStdString());
					return result;
				}
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
					result.content = RemoveLocalContent(save_path);
					if (result.content.status == LocalRemovalStatus::kFailed) {
						LOG_WARN("RemoveStopTask failed to remove downloaded content gid:{} error:{}",
								 gid.toStdString(), result.content.error.toStdString());
					}
					result.control_file = RemoveLocalContent(cache_file_path);
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
						bulk.Add(RemoveStopTaskResult(task, is_remove_file));
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
							double progress				  = 0.0;
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
