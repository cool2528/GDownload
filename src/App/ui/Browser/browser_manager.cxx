#include "browser_manager.h"
#include <QApplication>
#include <QFileInfo>
#include <QQmlEngine>
#include <nlohmann/json.hpp>
#include "Aria2CManager/engine_def.h"
#include "Definitions/appDef.h"
#include "logger.h"
namespace gdl {
	namespace ui {
		namespace browser {
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

			bool BrowserManager::AddHttpTask(const QString& url, const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value().toString();
					opt.emplace(key.toStdString(), value.toStdString());
				}
				auto res = engine::Aria2cDownloadManager::Instance().AddHttpTask(url.toStdString(), opt);
				return res.IsOk();
			}

			bool BrowserManager::AddTorrentTask(const QString& tarrent, const QVariantMap& options) {
				return false;
			}

			bool BrowserManager::AddMetalinkTask(const QString& metalink, const QVariantMap& options) {
				return false;
			}

			bool BrowserManager::Init() {
				auto res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2Responce, [this](const std::string& msg) { OnHandleAria2Message(msg); });
				if (res.HasError()) return false;
				subcription_ = res.Value();
				return true;
			}

			void BrowserManager::UnInit() {
				if (subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(subcription_);
				}
			}

			BrowserManager::BrowserManager(QObject* parent) : QObject(parent) {
				active_model_  = std::make_unique<DownloadTaskModel>();
				waiting_model_ = std::make_unique<DownloadTaskModel>();
				stoped_model_  = std::make_unique<DownloadTaskModel>();
				connect(this, &BrowserManager::sigUpdateTasksMessage, [this](const QString& result) {
					try {
						nlohmann::json array = nlohmann::json::parse(result.toStdString());
						if (!array.is_array()) {
							return;
						}
						for (const auto& object : array) {
							std::string status = object["status"].get<std::string>();
							DownloadTaskInfo task_info;
							QString task_id		  = QString::fromStdString(object["gid"].get<std::string>());
							auto completed_length = std::stoll(object["completedLength"].get<std::string>());
							auto connections	  = std::stoll(object["connections"].get<std::string>());
							auto download_speed	  = std::stoll(object["downloadSpeed"].get<std::string>());
							auto totalLength	  = std::stoll(object["totalLength"].get<std::string>());
							auto files			  = object["files"];
							QString file_path;
							for (const auto& file : files) {
								file_path = QString::fromStdString(file["path"].get<std::string>());
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
							else {}

							switch (task_info.task_state()) {
								case TaskState::kActive: {
									if (active_model_->ContainsTask(task_id)) {
										active_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										active_model_->AddTask(task_info);
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
								case TaskState::kError: {
									if (active_model_->ContainsTask(task_id)) {
										active_model_->RemoveTaskById(task_id);
									}
									if (waiting_model_->ContainsTask(task_id)) {
										waiting_model_->RemoveTaskById(task_id);
									}
									if (stoped_model_->ContainsTask(task_id)) {
										stoped_model_->UpdateTaskById(task_id, task_info);
									}
									else {
										stoped_model_->AddTask(task_info);
									}

								} break;

								default:
									break;
							}
						}
					} catch (std::exception& e) {
						LOG_ERR("{}", e.what());
					} catch (...) {}
				});
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
								Q_EMIT sigUpdateTasksMessage(QString::fromStdString(result.dump()));
								//LOG_DBG("OnHandleAria2Message  array {}", result.dump());
							}
						}
						else if (result.is_object()) {
							// object
							LOG_DBG("OnHandleAria2Message  object {}", result.dump());
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
						if (error_object.is_object() && error_object.find("message") != error_object.end()) {
							QString error_message = QString::fromStdString(error_object["message"].get<std::string>());
							Q_EMIT sigErrorMessage(error_message);
						}
					}
					else if (doc.find("method") != doc.end()) {
						// method message
					}
				} catch (std::exception& e) {
					LOG_ERR("{}", e.what());
				} catch (...) {
					LOG_ERR("OnHandleAria2Message exception");
				}
			}

			void RegisterTypes(QQmlEngine* engine) {
				qmlRegisterSingletonInstance<BrowserManager>(GEXPORT_MODULE_URL, 1, 0, "BrowserManager",
															 &BrowserManager::Instance());
				const QString app_path = QCoreApplication::applicationDirPath();
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
