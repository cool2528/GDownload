#include "NetWork_Disk_magager.h"
#include "Aria2CManager/aria2c_manager.h"
#include "Definitions/appDef.h"
#include "GDLCore/logger.h"
#include "PluginManager/plugin_manager.h"
#include "Settings/settings_manager.h"
#include "toast/toast_manager.h"
namespace gdl {
    namespace ui {
        namespace netdisk {
            NetWorkDiskManager::~NetWorkDiskManager() {}

            NetWorkDiskModel* NetWorkDiskManager::GetNetWorkDiskModel() {
                return model_.get();
            }

            void NetWorkDiskManager::ParseShareUrl(const QString& url) {
                const auto baidu_cokies = settings::Settings::Instance().GetBaiduPanCookies();
                auto task				= std::make_shared<ParseShareUrlTask>(url, baidu_cokies);
                worker_.AddTask(task);
            }

            void NetWorkDiskManager::ChangeDir(const QString& path, const QString& id) {
                auto task = std::make_shared<EnterDirectoryTask>(id, path);
                worker_.AddTask(task);
            }

            void NetWorkDiskManager::SelectAll() {
                if (!model_) {
                    LOG_ERR("model_ is nullptr")
                    return;
                }
                model_->SelectAll();
            }

            void NetWorkDiskManager::UnselectAll() {
                if (!model_) {
                    LOG_ERR("model_ is nullptr")
                    return;
                }
                model_->UnselectAll();
            }

            void NetWorkDiskManager::ToggleSelection(int index, bool is_selected) {
                if (!model_) {
                    LOG_ERR("model_ is nullptr")
                    return;
                }
                model_->ToggleSelection(index, is_selected);
            }

            void NetWorkDiskManager::DownloadSelectedFiles() {
                if (!model_) {
                    LOG_ERR("model_ is nullptr")
                    return;
                }
                auto selected_files = model_->GetSelectedFiles();
                if (selected_files.isEmpty()) {
                    return;
                }
                std::vector<INetDiskDownloadPlugin::FileInfo> file_infos;
                for (const auto& file : selected_files) {
                    INetDiskDownloadPlugin::FileInfo info;
                    info.name	 = file.file_name.toStdString();
                    info.path	 = file.file_path.toStdString();
                    info.file_id = file.file_id.toStdString();
                    info.is_dir	 = file.is_dir;
                    file_infos.push_back(info);
                }
                auto task = std::make_shared<GetDownloadInfoTask>(file_infos);
                worker_.AddTask(task);
            }

            NetWorkDiskManager::NetWorkDiskManager(QObject* parent) : QObject(parent) {
                model_ = std::make_unique<NetWorkDiskModel>();
                connect(&worker_, &AsyncTaskWorker::taskCompleted, this, &NetWorkDiskManager::HandleTaskResult);
            }

            void NetWorkDiskManager::HandleTaskResult(std::shared_ptr<TaskResult> result) {
                if (!result) {
                    return;
                }
                switch (result->taskType) {
                    case NetDiskTaskType::ParseShareUrl: {
                        auto parse_result = std::static_pointer_cast<ParseShareUrlResult>(result);
                        if (!parse_result) {
                            return;
                        }
                        if (parse_result->success) {
                            model_->Init(parse_result->files);
                        }
                        Q_EMIT taskFinished(parse_result->message, parse_result->success,
                                            static_cast<int>(NetDiskTaskType::ParseShareUrl));
                    } break;
                    case NetDiskTaskType::GetDownloadInfo: {
                        auto get_download_info_result = std::static_pointer_cast<GetDownloadInfoResult>(result);
                        if (!get_download_info_result) {
                            return;
                        }
                        if (get_download_info_result->success) {
                            auto download_info = get_download_info_result->downloadInfo;
                            for (const auto info : download_info) {
                                auto download_urls =
                                    info.mirrors.empty() ? std::vector<std::string>{info.real_url} : info.mirrors;
                                auto options = info.headers;
                                options.insert({"dir", settings::Settings::Instance().GetDir().toStdString()});
                                if (!info.file_name.empty()) {
                                    options.insert({"out", info.file_name});
                                }
                                auto res = gdl::engine::Aria2cDownloadManager::Instance().AddHttpTask(download_urls,
                                                                                                      options);
                                if (res.HasError()) {
                                    LOG_ERR("AddHttpTask error:{}", res.GetError().what());
                                    QString error_msg = QString("AddHttpTask %1 error: %2")
                                                            .arg(info.file_name.c_str(), res.GetError().what());
                                    toast::ToastManager::Instance().ShowError(error_msg);
                                    continue;
                                }
                            }
                        }
                        Q_EMIT taskFinished(get_download_info_result->message, get_download_info_result->success,
                                            static_cast<int>(NetDiskTaskType::GetDownloadInfo));
                    } break;
                    case NetDiskTaskType::EnterDirectory: {
                        auto enter_directory_result = std::static_pointer_cast<EnterDirectoryResult>(result);
                        if (!enter_directory_result) {
                            return;
                        }
                        if (enter_directory_result->success) {
                            model_->Init(enter_directory_result->files);
                        }
                        Q_EMIT taskFinished(enter_directory_result->message, enter_directory_result->success,
                                            static_cast<int>(NetDiskTaskType::EnterDirectory));
                    } break;
                    default:
                        break;
                }
            }

            void RegisterTypes(QQmlEngine* engine) {
                qmlRegisterSingletonInstance<NetWorkDiskManager>(GEXPORT_MODULE_URL, 1, 0, "NetWorkDiskManager",
                                                                 &NetWorkDiskManager::Instance());
            }

            AsyncTaskWorker::AsyncTaskWorker(QObject* parent) : QThread(parent) {
                start(QThread::LowPriority);
            }

            AsyncTaskWorker::~AsyncTaskWorker() {
                Stop();
                wait();
            }

            void AsyncTaskWorker::AddTask(std::shared_ptr<NetDiskTask> task) {
                QMutexLocker locker(&mutex_);
                task_queue_.emplace(task);
                condition_.wakeOne();
            }

            void AsyncTaskWorker::Stop() {
                QMutexLocker locker(&mutex_);
                stopped_ = true;
                condition_.wakeAll();
            }

            void AsyncTaskWorker::run() {
                while (!stopped_) {
                    std::shared_ptr<NetDiskTask> task;
                    {
                        QMutexLocker locker(&mutex_);
                        while (!stopped_ && task_queue_.empty()) {
                            condition_.wait(&mutex_);
                        }
                        if (task_queue_.empty()) {
                            continue;
                        }
                        task = task_queue_.front();
                        task_queue_.pop();
                    }
                    if (task) {
                        auto result = ExecuteTask(task);
                        if (result) {
                            Q_EMIT taskCompleted(result);
                        }
                        else {
                            switch (task->type) {
                                case NetDiskTaskType::ParseShareUrl: {
                                    auto parse_result = std::make_shared<ParseShareUrlResult>(
                                        false, tr("Failed to parse the share link."),
                                        std::vector<INetDiskDownloadPlugin::FileInfo>{});
                                    Q_EMIT taskCompleted(parse_result);
                                } break;
                                case NetDiskTaskType::GetDownloadInfo: {
                                    auto get_download_info_result = std::make_shared<GetDownloadInfoResult>(
                                        false, tr("Failed to retrieve download link information."),
                                        std::vector<INetDiskDownloadPlugin::ParseResult>{});
                                    Q_EMIT taskCompleted(get_download_info_result);
                                } break;
                                case NetDiskTaskType::EnterDirectory: {
                                    auto enter_directory_result = std::make_shared<EnterDirectoryResult>(
                                        false, tr("Failed to switch directory."),
                                        std::vector<INetDiskDownloadPlugin::FileInfo>{}, "");
                                    Q_EMIT taskCompleted(enter_directory_result);
                                } break;

                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            std::shared_ptr<TaskResult> AsyncTaskWorker::ExecuteTask(std::shared_ptr<NetDiskTask> task) {
                auto plugin_vec = plugin::DownloadPluginManager::Instance().GetPluginsForUrl("https://pan.baidu.com");
                if (plugin_vec.empty()) {
                    return nullptr;
                }
                auto plugin = plugin_vec.front();
                if (!plugin) {
                    return nullptr;
                }
                try {
                    switch (task->type) {
                        case NetDiskTaskType::ParseShareUrl: {
                            auto parse_task = std::static_pointer_cast<ParseShareUrlTask>(task);
                            if (!parse_task) {
                                return nullptr;
                            }
                            auto result =
                                plugin->ParseUrl(parse_task->url.toStdString(), parse_task->userToken.toStdString());
                            if (!result.has_value()) return nullptr;
                            return std::make_shared<ParseShareUrlResult>(true, "succeed", result.value());
                        }
                        case NetDiskTaskType::GetDownloadInfo: {
                            auto get_download_info_task = std::static_pointer_cast<GetDownloadInfoTask>(task);
                            if (!get_download_info_task) {
                                return nullptr;
                            }
                            std::vector<INetDiskDownloadPlugin::ParseResult> parse_results;
                            for (const auto& file : get_download_info_task->files) {
                                auto result = plugin->GetDownloadInfo(file);
                                if (!result.has_value()) return nullptr;
                                parse_results.insert(parse_results.end(), result.value().begin(), result.value().end());
                            }
                            return std::make_shared<GetDownloadInfoResult>(true, "succeed", parse_results);
                        }
                        case NetDiskTaskType::EnterDirectory: {
                            auto enter_directory_task = std::static_pointer_cast<EnterDirectoryTask>(task);
                            if (!enter_directory_task) {
                                return nullptr;
                            }
                            INetDiskDownloadPlugin::FileInfo info;
                            info.file_id = enter_directory_task->fileId.toStdString();
                            info.path	 = enter_directory_task->path.toStdString();
                            auto result	 = plugin->EnterDirectory(info);
                            if (!result.has_value()) return nullptr;
                            return std::make_shared<EnterDirectoryResult>(true, "succeed", result.value(),
                                                                          enter_directory_task->path);
                        }
                        default:
                            break;
                    }
                } catch (std::exception& e) {
                    LOG_ERR("AsyncTaskWorker::ExecuteTask failed, error: {}", e.what());
                    return nullptr;
                }
                return nullptr;
            }
        }  // namespace netdisk
    }  // namespace ui
}  // namespace gdl
