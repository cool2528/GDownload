#include "browser_manager.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QQmlEngine>
#include <nlohmann/json.hpp>
#include "Aria2CManager/engine_def.h"
#include "Definitions/appDef.h"
#include "Parser/file_parser.h"
#include "PluginManager/plugin_manager.h"
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
			BrowserManager* BrowserManager::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
				Q_UNUSED(qmlengine)
				Q_UNUSED(jsengine);
				return &BrowserManager::Instance();
			}

			BrowserManager::~BrowserManager() {}

			DownloadTaskModel* BrowserManager::GetActiveDownloadModel() {
				return active_model_.get();
			}

			DownloadTaskModel* BrowserManager::GetStopedDownloadModel() {
				return stoped_model_.get();
			}

			DownloadTaskModel* BrowserManager::GetWaitingDownloadModel() {
				return waiting_model_.get();
			}

			bool BrowserManager::AddHttpTask(const QVariantList& urls, const QVariantMap& options) {
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
						auto res =
                            engine::Aria2cDownloadManager::Instance().AddHttpTask({url.toString().toStdString()}, opt);
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

			bool BrowserManager::AddTorrentTask(const QString& tarrent, const QVariantMap& options) {
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

			bool BrowserManager::AddMetalinkTask(const QString& metalink, const QVariantMap& options) {
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

			bool BrowserManager::PauseTask(int page_index, const QString& gid) {
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

			bool BrowserManager::PauseAllTask(int page_index) {
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

			bool BrowserManager::ForcePauseTask(int page_index, const QString& gid) {
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

			bool BrowserManager::ForcePauseAllTask() {
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kForcePauseAll)
					.IsOk();
			}

			bool BrowserManager::UnpauseTask(int page_index, const QString& gid) {
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

			bool BrowserManager::UnpauseAllTask(int page_index) {
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

			bool BrowserManager::RemoveTask(int page_index, const QString& gid) {
				if (gid.isEmpty()) return false;
                if (page_index != 0 && page_index != 1) {
					return false;
				}
				if (active_model_) {
					active_model_->RemoveTaskById(gid);
				}
				if (waiting_model_) {
					waiting_model_->RemoveTaskById(gid);
				}
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
				return true;
			}

			bool BrowserManager::RemoveAllTask(int page_index, bool is_remove_file) {
				if (page_index == 0) {
					if (active_model_) {
						for (const auto& task : active_model_->GetTaskIds()) {
							auto task_info = active_model_->GetTaskById(task);
							if (!task_info) {
								continue;
							}
							QString save_path = task_info->task_save_path();
							RemoveTask(page_index, task);
							if (is_remove_file && QFile::exists(save_path)) {
								QFile::remove(save_path);
							}
						}
						return true;
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						for (const auto& task : waiting_model_->GetTaskIds()) {
							auto task_info = waiting_model_->GetTaskById(task);
							if (!task_info) {
								continue;
							}
							QString save_path = task_info->task_save_path();
							RemoveTask(page_index, task);
							if (is_remove_file && QFile::exists(save_path)) {
								QFile::remove(save_path);
							}
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

			bool BrowserManager::ForceRemoveTask(const QString& gid) {
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

			bool BrowserManager::RemoveDownloadResult(const QString& gid) {
				if (gid.isEmpty()) return false;
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kRemoveDownloadResult, gid.toStdString())
					.IsOk();
			}

			bool BrowserManager::PurgeDownloadResult() {
				return engine::Aria2cDownloadManager::Instance()
					.CallAria2cMethod(engine::Aria2Method::kPurgeDownloadResult)
					.IsOk();
			}

			bool BrowserManager::ChangeOption(const QString& gid, const QVariantMap& options) {
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

			bool BrowserManager::ChangeGlobalOption(const QVariantMap& options) {
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

			void BrowserManager::OpenFileLocation(const QString& file_path) {
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

			bool BrowserManager::RemoveStopTask(const QString& gid, bool is_remove_file) const {
				if (gid.isEmpty()) return false;
				if (stoped_model_) {
					const auto task = stoped_model_->GetTaskById(gid);
					if (!task) {
						return false;
					}
                    const QString save_path		  = task->task_save_path();
                    const QString cache_file_path = save_path + ".aria2";
                    const auto res				  = stoped_model_->RemoveTaskById(gid);
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

			bool BrowserManager::RemoveStopTask(int index, bool is_remove_file) const {
				if (stoped_model_) {
					auto task = stoped_model_->GetTask(index);
					if (!task) {
						return false;
					}
                    QString gid					  = task->task_id();
                    const QString save_path		  = task->task_save_path();
                    const QString cache_file_path = save_path + ".aria2";
                    const auto res				  = stoped_model_->RemoveTask(index);
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

			bool BrowserManager::RemoveAllStopTask(bool is_remove_file) const {
				if (stoped_model_) {
					auto tasks = stoped_model_->GetTaskIds();
					bool res   = false;
					for (const auto& task : tasks) {
						res = RemoveStopTask(task, is_remove_file);
					}
					return res;
				}
				return false;
			}

			void BrowserManager::RefreshTaskList(int page_index) {
				if (page_index == 0) {
					if (active_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellActive,
																				   keys);
					}
				}
				else if (page_index == 1) {
					if (waiting_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellWaiting,
																				   keys);
					}
				}
				else if (page_index == 2) {
					if (stoped_model_) {
						engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kTellStopped,
																				   keys);
					}
				}
				else {
					LOG_ERR("RefreshTaskList error: page_index is invalid");
				}
			}

			parser::FilePreviewModel* BrowserManager::GetFilePreviewModel(const QString& file_path) {
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

			bool BrowserManager::Init() {
				// subscribe aria2 responce
				auto res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2Responce, [this](const std::string& msg) { OnHandleAria2Message(msg); });
				if (res.HasError()) return false;
				aria2_responce_subcription_ = res.Value();
				// subscribe active progress
				res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2ActiveProgress, [this](const std::string& msg) { OnHandleAria2ActiveProgress(msg); });
				if (res.HasError()) return false;
				aria2_active_progress_subcription_ = res.Value();
				return true;
			}

			void BrowserManager::UnInit() {
				if (aria2_responce_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(aria2_responce_subcription_);
				}
				if (aria2_active_progress_subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(
						aria2_active_progress_subcription_);
				}
			}

			gdl::cache::DownloadRecord BrowserManager::DownloadTaskInfoToRecord(const DownloadTaskInfo& info) {
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

			DownloadTaskInfo BrowserManager::DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record) {
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

			BrowserManager::BrowserManager(QObject* parent) : QObject(parent) {
				active_model_  = std::make_unique<DownloadTaskModel>();
				waiting_model_ = std::make_unique<DownloadTaskModel>();
				stoped_model_  = std::make_unique<DownloadTaskModel>();
				InitDownloadHistoryCache();
				connect(
					this, &BrowserManager::sigUpdateTasksMessage, this,
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
								} break;
								case TaskState::kPause: {
									if (active_model_->ContainsTask(task_id)) {
										active_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										active_model_->AddTask(task_info);
									}

									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										waiting_model_->AddTask(task_info);
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
									if (stoped_model_->ContainsTask(task_id)) {
										stoped_model_->UpdateTaskById(task_id, task_info);
										gdl::cache::DownloadRecord record = DownloadTaskInfoToRecord(task_info);
										if (!gdl::cache::DownloadHistoryCache::Instance().UpdateRecord(record)) {
											LOG_ERR("Failed to UPDATE record to history cache {}", record.save_path);
										}
									}
									else {
										stoped_model_->AddTask(task_info);
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
					this, &BrowserManager::sigUpdateActiveProgress, this,
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
                    this, &BrowserManager::sigErrorMessage, this,
                    [this](const QString& message) { toast::ToastManager::Instance().ShowError(message); },
                    Qt::QueuedConnection);
			}
			void BrowserManager::InitDownloadHistoryCache() const {
				const auto records = gdl::cache::DownloadHistoryCache::Instance().GetRecords();
				for (const auto& record : records) {
					DownloadTaskInfo info = DownloadRecordToTaskInfo(record);
					if (stoped_model_ && !stoped_model_->ContainsTask(info.task_id())) {
						if (QFile::exists(info.task_save_path())) {
							stoped_model_->AddTask(info);
						}
						else {
							gdl::cache::DownloadHistoryCache::Instance().DeleteRecord(info.task_id().toStdString());
						}
					}
				}
			}
			void BrowserManager::OnHandleAria2Message(const std::string& msg) {
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
									std::string status = object["status"].get<std::string>();
									DownloadTaskInfo task_info;
									QString task_id		  = QString::fromStdString(object["gid"].get<std::string>());
									auto completed_length = std::stoll(object["completedLength"].get<std::string>());
									auto connections	  = std::stoll(object["connections"].get<std::string>());
									auto download_speed	  = std::stoll(object["downloadSpeed"].get<std::string>());
									auto totalLength	  = std::stoll(object["totalLength"].get<std::string>());
									auto files			  = object["files"];
									QString file_path, download_url;
									for (const auto& file : files) {
										file_path = QString::fromStdString(file["path"].get<std::string>());
										if (file.contains("uris") && file["uris"].is_array()) {
											auto uris = file["uris"];
											for (const auto& uri : uris) {
												download_url = QString::fromStdString(uri["uri"].get<std::string>());
												break;
											}
										}
										if (!file_path.isEmpty()) break;
									}
									if (file_path.isEmpty()) return;
									task_info.set_task_download_speed(download_speed);
									task_info.set_task_id(task_id);
									task_info.set_task_current_size(completed_length);
									task_info.set_task_total_size(totalLength);
									task_info.set_task_connections(connections);
									task_info.set_task_file_name(QFileInfo(file_path).fileName());
									task_info.set_task_save_path(file_path);
									task_info.set_task_download_link(download_url);
									if (status == "active") {
										task_info.set_task_state(TaskState::kActive);
									}
									else if (status == "waiting") {
										task_info.set_task_state(TaskState::kWaiting);
									}
									else if (status == "complete") {
										task_info.set_task_state(TaskState::kComplete);
									}
									else if (status == "paused") {
										task_info.set_task_state(TaskState::kPause);
									}
									else if (status == "error") {
										task_info.set_task_state(TaskState::kError);
									}
									else if (status == "removed") {
										task_info.set_task_state(TaskState::kRemoved);
									}
									else {
										LOG_WARN("Unknown state type: {}", status);
									}
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

			void BrowserManager::OnHandleAria2ActiveProgress(const std::string& msg) {
				try {
					nlohmann::json doc = nlohmann::json::parse(msg);
					if (doc.is_object()) {
						if (doc.contains("totalLength") && doc.contains("completedLength")) {
							std::int64_t total_length	  = doc["totalLength"].get<std::int64_t>();
							std::int64_t completed_length = doc["completedLength"].get<std::int64_t>();
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

			DownloadTaskInfo BrowserManager::Aria2QueryByGidTaskInfo(const std::string& gid) {
				DownloadTaskInfo task_info;
				const std::string host = std::string("http://127.0.0.1:") + kEngineRpcPort;
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
								std::string status	  = object["status"].get<std::string>();
								QString task_id		  = QString::fromStdString(object["gid"].get<std::string>());
								auto completed_length = std::stoll(object["completedLength"].get<std::string>());
								auto connections	  = std::stoll(object["connections"].get<std::string>());
								auto download_speed	  = std::stoll(object["downloadSpeed"].get<std::string>());
								auto totalLength	  = std::stoll(object["totalLength"].get<std::string>());
								auto files			  = object["files"];
								QString file_path, download_url;
								for (const auto& file : files) {
									file_path = QString::fromStdString(file["path"].get<std::string>());
									for (const auto& file : files) {
										file_path = QString::fromStdString(file["path"].get<std::string>());
										if (file.contains("uris") && file["uris"].is_array()) {
											auto uris = file["uris"];
											for (const auto& uri : uris) {
												download_url = QString::fromStdString(uri["uri"].get<std::string>());
												break;
											}
										}
										if (!file_path.isEmpty()) break;
									}
									if (!file_path.isEmpty()) break;
								}
								if (file_path.isEmpty()) {
									LOG_WARN("Failed to get file path by gid:{}", gid)
									return task_info;
								}
								task_info.set_task_download_speed(download_speed);
								task_info.set_task_id(task_id);
								task_info.set_task_current_size(completed_length);
								task_info.set_task_total_size(totalLength);
								task_info.set_task_connections(connections);
								task_info.set_task_file_name(QFileInfo(file_path).fileName());
								task_info.set_task_save_path(file_path);
								task_info.set_task_download_link(download_url);
								if (status == "active") {
									task_info.set_task_state(TaskState::kActive);
								}
								else if (status == "waiting") {
									task_info.set_task_state(TaskState::kWaiting);
								}
								else if (status == "complete") {
									task_info.set_task_state(TaskState::kComplete);
								}
								else if (status == "paused") {
									task_info.set_task_state(TaskState::kPause);
								}
								else if (status == "error") {
									task_info.set_task_state(TaskState::kError);
								}
								else if (status == "removed") {
									task_info.set_task_state(TaskState::kRemoved);
								}
								else {
									LOG_WARN("Unknown state type: {}", status);
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

			void RegisterTypes(QQmlEngine* engine) {
                qmlRegisterSingletonInstance<BrowserManager>(GEXPORT_MODULE_URL, 1, 0, "BrowserManager",
                                                             &BrowserManager::Instance());
				const QString app_path = QString::fromStdString(os::GetExecutableDir());
				QString aria2c_engine_path;
#ifdef __APPLE__
				QString app_path_dir =
					QString::fromStdString(std::filesystem::path(app_path.toStdString()).parent_path().string());
				aria2c_engine_path = app_path_dir + "/Resources/engine/aria2c";
#elif _WIN32 || defined(_WIN64)
				aria2c_engine_path = app_path + "/engine/aria2c.exe";
#else
				aria2c_engine_path = app_path + "/engine/aria2c";
#endif
				gdl::engine::Aria2cDownloadManager::Instance().InitAria2cEngine(aria2c_engine_path.toStdString());
				BrowserManager::Instance().Init();
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
