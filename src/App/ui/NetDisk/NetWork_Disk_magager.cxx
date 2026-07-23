#include "NetWork_Disk_magager.h"
#include "Aria2CManager/aria2c_manager.h"
#include "Definitions/appDef.h"
#include "GDLCore/logger.h"
#include "PluginManager/plugin_manager.h"
#include "PluginMarket/plugin_config_manager.h"
#include "Settings/settings_manager.h"
#include "toast/toast_manager.h"
#include "verification_bridge.h"
namespace gdl {
    namespace ui {
        namespace netdisk {
            namespace {
                // 按任务类型构造失败结果（统一错误出口）
                std::shared_ptr<TaskResult> MakeFailureResult(NetDiskTaskType type, const QString& msg) {
                    switch (type) {
                        case NetDiskTaskType::ParseShareUrl:
                            return std::make_shared<ParseShareUrlResult>(false, msg,
                                                                         std::vector<INetDiskDownloadPlugin::FileInfo>{});
                        case NetDiskTaskType::EnterDirectory:
                            return std::make_shared<EnterDirectoryResult>(
                                false, msg, std::vector<INetDiskDownloadPlugin::FileInfo>{}, "");
                        case NetDiskTaskType::GetDownloadInfo:
                            return std::make_shared<GetDownloadInfoResult>(
                                false, msg, std::vector<INetDiskDownloadPlugin::ParseResult>{});
                    }
                    return nullptr;
                }
            }  // namespace

            NetWorkDiskManager::~NetWorkDiskManager() {}

            NetWorkDiskModel* NetWorkDiskManager::GetNetWorkDiskModel() {
                return model_.get();
            }

            QVariantList NetWorkDiskManager::MatchPlugins(const QString& url) {
                QVariantList result;
                auto plugins = plugin::DownloadPluginManager::Instance().GetPluginsForUrl(url.toStdString());
                for (const auto& plugin : plugins) {
                    if (!plugin) {
                        continue;
                    }
                    const auto name = QString::fromStdString(plugin->GetPluginMetadata().name);
                    result.push_back(market::PluginConfigManager::Instance().pluginInfo(name));
                }
                return result;
            }

            void NetWorkDiskManager::ParseShareUrl(const QString& url, const QString& pluginName) {
                // userToken 取该插件 role=token 配置字段的当前值（声明式 Schema）
                const auto token = market::PluginConfigManager::Instance().TokenFor(pluginName);
                auto task		 = std::make_shared<ParseShareUrlTask>(url, token);
                task->plugin_name = pluginName.toStdString();
                worker_.AddTask(task);
            }

            void NetWorkDiskManager::ChangeDir(const QString& path, const QString& id) {
                auto task = std::make_shared<EnterDirectoryTask>(id, path);
                task->plugin_name = current_plugin_name_.toStdString();
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
                task->plugin_name = current_plugin_name_.toStdString();
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
                            current_plugin_name_ = parse_result->pluginName;
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
                qmlRegisterSingletonInstance<VerificationBridge>(GEXPORT_MODULE_URL, 1, 0, "VerificationBridge",
                                                                 &VerificationBridge::Instance());
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
                // 按名取插件（解析时 UI 传入，浏览/下载沿用会话绑定的插件名）
                auto plugin = plugin::DownloadPluginManager::Instance().GetPluginByName(task->plugin_name);
                if (!plugin) {
                    LOG_ERR("netdisk plugin not found: {}", task->plugin_name);
                    return MakeFailureResult(task->type,
                                             tr("The plugin is no longer available. Check the Plugin Market."));
                }
                // 幂等挂接验证输入与消息通知回调（每次任务前设置，兼容插件热重载）
                plugin->SetVerificationCallback([](INetDiskDownloadPlugin::VerificationCallbackParam& param) {
                    VerificationBridge::Instance().Request(param);
                });
                plugin->SetMessageNotifyCallback(
                    [](std::string_view message, const INetDiskDownloadPlugin::MsgType& type) {
                        // 回调发生在 worker 线程，转投 UI 线程展示 toast
                        QString msg = QString::fromUtf8(message.data(), static_cast<int>(message.size()));
                        // 插件侧约定的标准英文提示映射为可翻译文本；其余（如网盘 API 原文）原样透传
                        if (msg == QLatin1String("Parsing cancelled: the share link requires an extraction code.")) {
                            msg = tr("Parsing cancelled: the share link requires an extraction code.");
                        }
                        else if (msg == QLatin1String("Download cancelled: the share link requires an extraction code.")) {
                            msg = tr("Download cancelled: the share link requires an extraction code.");
                        }
                        else if (msg == QLatin1String("Extraction code verification failed.")) {
                            msg = tr("Extraction code verification failed.");
                        }
                        auto* toast_manager = &toast::ToastManager::Instance();
                        QMetaObject::invokeMethod(
                            toast_manager,
                            [toast_manager, msg, type]() {
                                switch (type) {
                                    case INetDiskDownloadPlugin::MsgType::kError:
                                        toast_manager->ShowError(msg);
                                        break;
                                    case INetDiskDownloadPlugin::MsgType::kWarning:
                                        toast_manager->ShowWarning(msg);
                                        break;
                                    case INetDiskDownloadPlugin::MsgType::kSuccess:
                                        toast_manager->ShowSuccess(msg);
                                        break;
                                    case INetDiskDownloadPlugin::MsgType::kDebug:
                                        // debug 级别仅记日志场景，不打扰用户
                                        break;
                                    default:
                                        toast_manager->ShowInfo(msg);
                                        break;
                                }
                            },
                            Qt::QueuedConnection);
                    });
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
                            return std::make_shared<ParseShareUrlResult>(true, "succeed", result.value(),
                                                                         QString::fromStdString(task->plugin_name));
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
