#include "browser_manager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QQmlEngine>
#include <nlohmann/json.hpp>
#include "Aria2CManager/engine_def.h"
#include "Browser/download_url_utils.h"
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
				// 从单个 aria2 任务对象解析出 DownloadTaskInfo,统一 OnHandleAria2Message 与
				// Aria2QueryByGidTaskInfo 两处重复逻辑。task_id 为空表示对象无有效路径(应跳过)。
				DownloadTaskInfo ParseAria2TaskObject(const nlohmann::json& object) {
					DownloadTaskInfo task_info;
					if (!object.is_object() || !object.contains("gid")) {
						return task_info;
					}
					// aria2 数值字段以字符串下发,容错解析为 0
					auto parse_ll = [&object](const char* key) -> std::int64_t {
						if (object.contains(key) && object[key].is_string()) {
							try {
								return std::stoll(object[key].get<std::string>());
							} catch (...) {
							}
						}
						return 0;
					};

					const std::string status = object.value("status", std::string());
					const QString task_id = QString::fromStdString(object["gid"].get<std::string>());
					const std::int64_t completed = parse_ll("completedLength");
					const std::int64_t connections = parse_ll("connections");
					const std::int64_t speed = parse_ll("downloadSpeed");
					const std::int64_t total_length = parse_ll("totalLength");

					// 收集文件路径与下载源
					QString first_path, download_url;
					int file_count = 0;
					if (object.contains("files") && object["files"].is_array()) {
						const auto& files = object["files"];
						file_count = static_cast<int>(files.size());
						for (const auto& file : files) {
							if (first_path.isEmpty() && file.contains("path")) {
								first_path = QString::fromStdString(file["path"].get<std::string>());
							}
							if (download_url.isEmpty() && file.contains("uris") && file["uris"].is_array()) {
								for (const auto& uri : file["uris"]) {
									if (uri.contains("uri")) {
										download_url = QString::fromStdString(uri["uri"].get<std::string>());
										break;
									}
								}
							}
						}
					}

					// 种子名(多文件任务用作显示名与根目录)
					QString torrent_name;
					if (object.contains("bittorrent") && object["bittorrent"].is_object()) {
						const auto& bt = object["bittorrent"];
						if (bt.contains("info") && bt["info"].is_object() && bt["info"].contains("name")) {
							torrent_name = QString::fromStdString(bt["info"]["name"].get<std::string>());
						}
					}
					QString task_dir;
					if (object.contains("dir")) {
						task_dir = QString::fromStdString(object["dir"].get<std::string>());
					}

					QString file_name, save_path;
					if (file_count > 1) {
						// 多文件 BT:显示种子名,保存路径落到种子根目录(dir/种子名),"打开目录"落到种子根
						if (!torrent_name.isEmpty() && !task_dir.isEmpty()) {
							save_path = task_dir + "/" + torrent_name;
							file_name = torrent_name;
						} else if (!task_dir.isEmpty()) {
							save_path = task_dir;
							file_name = QFileInfo(first_path).fileName();
						} else {
							save_path = first_path;
							file_name = QFileInfo(first_path).fileName();
						}
					} else {
						// 单文件:沿用第一个文件的完整路径
						save_path = first_path;
						file_name = QFileInfo(first_path).fileName();
					}

					if (save_path.isEmpty()) {
						return task_info;  // 无有效路径,task_id 保持为空,调用方跳过
					}

					task_info.set_task_id(task_id);
					task_info.set_task_download_speed(speed);
					task_info.set_task_current_size(completed);
					task_info.set_task_total_size(total_length);
					task_info.set_task_connections(connections);
					task_info.set_task_file_name(file_name);
					task_info.set_task_save_path(save_path);
					task_info.set_task_download_link(download_url);

					if (status == "active") {
						task_info.set_task_state(TaskState::kActive);
					} else if (status == "waiting") {
						task_info.set_task_state(TaskState::kWaiting);
					} else if (status == "complete") {
						task_info.set_task_state(TaskState::kComplete);
					} else if (status == "paused") {
						task_info.set_task_state(TaskState::kPause);
					} else if (status == "error") {
						task_info.set_task_state(TaskState::kError);
					} else if (status == "removed") {
						task_info.set_task_state(TaskState::kRemoved);
					} else {
						LOG_WARN("Unknown state type: {}", status);
					}
					return task_info;
				}
			}  // namespace

			BrowserManager* BrowserManagerImpl::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
				Q_UNUSED(qmlengine)
				Q_UNUSED(jsengine);
				return &BrowserManagerImpl::Instance();
			}

			BrowserManagerImpl::~BrowserManagerImpl() {}

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
						auto res =
                            engine::Aria2cDownloadManager::Instance().AddHttpTask({normalized_url->toStdString()}, opt);
						if (res.HasError()) {
							LOG_ERR("Failed to add HTTP download task Download address {} error {}",
									url.toString().toStdString(), res.GetError().what());
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
				if (!QFile::exists(tarrent)) return false;
				// 读取tarrent文件到base64
				QFile file(tarrent);
				if (!file.open(QIODevice::ReadOnly)) return false;
				QByteArray data = file.readAll();
				file.close();
				std::string base64_data = data.toBase64().toStdString();

				auto res = engine::Aria2cDownloadManager::Instance().AddTorrentTask(base64_data, opt);
				if (res.HasError()) {
					LOG_ERR("Failed to add Torrent download task Download address {} error {}", tarrent.toStdString(),
							res.GetError().what());
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
				if (!QFile::exists(metalink)) return false;
				// 读取metalink文件到base64
				QFile file(metalink);
				if (!file.open(QIODevice::ReadOnly)) return false;
				QByteArray data = file.readAll();
				file.close();
				std::string base64_data = data.toBase64().toStdString();

				auto res = engine::Aria2cDownloadManager::Instance().AddMetalinkTask(base64_data, opt);
				if (res.HasError()) {
					LOG_ERR("Failed to add metalink download task Download address {} error {}", metalink.toStdString(),
							res.GetError().what());
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

			bool BrowserManagerImpl::RemoveTask(int page_index, const QString& gid, bool is_remove_file) {
				if (gid.isEmpty()) return false;
                if (page_index != 0 && page_index != 1) {
					return false;
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

				// 从模型中移除任务
				if (active_model_) {
					active_model_->RemoveTaskById(gid);
				}
				if (waiting_model_) {
					waiting_model_->RemoveTaskById(gid);
				}

				// 调用 aria2 删除任务
				const auto res = engine::Aria2cDownloadManager::Instance()
									 .CallAria2cMethod(engine::Aria2Method::kRemove, gid.toStdString())
									 .IsOk();
				if (res) {
					engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
						engine::Aria2Method::kRemoveDownloadResult, gid.toStdString());
					if (active_model_) {
						active_model_->RemoveTaskById(gid);
					}
				}

				// 如果需要删除文件
				if (is_remove_file && !save_path.isEmpty()) {
					if (QFile::exists(save_path)) {
						QFile::remove(save_path);
					}
					if (QFile::exists(cache_file_path)) {
						QFile::remove(cache_file_path);
					}
				}

				return true;
			}

			bool BrowserManagerImpl::RemoveAllTask(int page_index, bool is_remove_file) {
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							RemoveTask(page_index, task, is_remove_file);
						}
						return true;
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							RemoveTask(page_index, task, is_remove_file);
						}
						return true;
					}
				}
				else if (page_index == 2) {
					return RemoveAllStopTask(is_remove_file);
				}
				else {
					LOG_ERR("RemoveAllTask error: page_index is invalid");
				}
				return false;
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

			bool BrowserManagerImpl::RemoveStopTask(const QString& gid, bool is_remove_file) const {
				if (gid.isEmpty()) return false;
				if (stopped_model_) {
					const auto task = stopped_model_->GetTaskById(gid);
					if (!task) {
						return false;
					}
                    const QString save_path		  = task->task_save_path();
                    const QString cache_file_path = save_path + ".aria2";
                    const auto res				  = stopped_model_->RemoveTaskById(gid);
					gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(gid.toStdString());
					if (is_remove_file) {
						if (QFile::exists(save_path)) {
							QFile::remove(save_path);
						}
                        if (QFile::exists(cache_file_path)) {
                            QFile::remove(cache_file_path);
                        }
					}
					return res;
				}
				return false;
			}

			bool BrowserManagerImpl::RemoveStopTask(int index, bool is_remove_file) const {
				if (stopped_model_) {
					auto task = stopped_model_->GetTask(index);
					if (!task) {
						return false;
					}
                    QString gid					  = task->task_id();
                    const QString save_path		  = task->task_save_path();
                    const QString cache_file_path = save_path + ".aria2";
                    const auto res				  = stopped_model_->RemoveTask(index);
					gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(gid.toStdString());
					if (is_remove_file) {
						if (QFile::exists(save_path)) {
							QFile::remove(save_path);
						}
                        if (QFile::exists(cache_file_path)) {
                            QFile::remove(cache_file_path);
                        }
					}
					return res;
				}
				return false;
			}

			bool BrowserManagerImpl::RemoveAllStopTask(bool is_remove_file) const {
				if (stopped_model_) {
					auto tasks = stopped_model_->GetTaskIds();
					bool res   = false;
					for (const auto& task : tasks) {
						res = RemoveStopTask(task, is_remove_file);
					}
					return res;
				}
				return false;
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
				if (res.HasError()) return false;
				aria2_active_progress_subcription_ = res.Value();
				// subscribe sync server list
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2SyncMagnetServerList, [this](const std::string& msg) { 
						Q_EMIT sigUpdateSyncServerList(QString::fromStdString(msg));
					});
				if (res.HasError()) return false;
				aria2_sync_server_list_subcription_ = res.Value();
				// subscribe tracker update status
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2TrackerUpdateStatus, [this](const std::string& msg) { OnHandleTrackerUpdateStatus(msg); });
				if (res.HasError()) return false;
				aria2_tracker_update_status_subscription_ = res.Value();
				return true;
			}

			void BrowserManagerImpl::UnInit() {
				if (aria2_responce_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(aria2_responce_subcription_);
				}
				if (aria2_active_progress_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_active_progress_subcription_);
				}
				if (aria2_sync_server_list_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_sync_server_list_subcription_);
				}
				if (aria2_tracker_update_status_subscription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_tracker_update_status_subscription_);
				}
			}

			gdl::cache::DownloadRecord BrowserManagerImpl::DownloadTaskInfoToRecord(const DownloadTaskInfo& info) {
				gdl::cache::DownloadRecord record;
				record.completed_time  = std::time(nullptr);
				record.created_time	   = std::time(nullptr);
				record.connections	   = info.task_connections();
				record.download_speed  = info.task_download_speed();
				record.download_url	   = info.task_download_link().toStdString();
				record.downloaded_size = info.task_current_size();
				record.task_id		   = info.task_id().toStdString();
				record.file_name	   = info.task_file_name().toStdString();
				record.save_path	   = info.task_save_path().toStdString();
				record.total_size	   = info.task_total_size();
				record.state		   = static_cast<gdl::cache::DownloadState>(info.task_state());
				return record;
			}

			DownloadTaskInfo BrowserManagerImpl::DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record) {
				DownloadTaskInfo info;
				info.set_task_id(QString::fromStdString(record.task_id));
				info.set_task_file_name(QString::fromStdString(record.file_name));
				info.set_task_save_path(QString::fromStdString(record.save_path));
				info.set_task_download_link(QString::fromStdString(record.download_url));
				info.set_task_current_size(record.downloaded_size);
				info.set_task_total_size(record.total_size);
				info.set_task_connections(record.connections);
				info.set_task_download_speed(record.download_speed);
				info.set_task_state(static_cast<TaskState>(record.state));
				return info;
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
										if (!gdl::cache::DownloadHistoryCache::Instance().UpdateRecord(record)) {
											LOG_ERR("Failed to UPDATE record to history cache {}", record.save_path);
										}
									}
									else {
										stopped_model_->AddTask(task_info);
										gdl::cache::DownloadRecord record = DownloadTaskInfoToRecord(task_info);
										if (!gdl::cache::DownloadHistoryCache::Instance().AddRecord(record)) {
											LOG_ERR("Failed to add record to history cache {}", record.save_path);
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
				const auto records = gdl::cache::DownloadHistoryCache::Instance().GetRecords();
				for (const auto& record : records) {
					DownloadTaskInfo info = DownloadRecordToTaskInfo(record);
					if (stopped_model_ && !stopped_model_->ContainsTask(info.task_id())) {
						if (QFile::exists(info.task_save_path())) {
							stopped_model_->AddTask(info);
						}
						else {
							gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(info.task_id().toStdString());
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
						auto get_params_task = [](const nlohmann::json& param) {
							DownloadTaskInfo task_info;
							for (const auto& item : param) {
								std::string gid = item["gid"].get<std::string>();
								task_info		= Aria2QueryByGidTaskInfo(gid);
							}
							return task_info;
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
							auto task = get_params_task(params);
							if (task.task_id().isEmpty()) {
								LOG_WARN("Failed to get task info by gid");
								return;
							}
							task.set_task_state(TaskState::kError);
							Q_EMIT sigUpdateTasksMessage(task);

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
