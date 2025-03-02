#include "linux_updater.h"
#if defined(__linux__)
#include <appimage/update.h>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <chrono>
#include <thread>

namespace gdl {
    namespace update {

        LinuxUpdater::LinuxUpdater() = default;

        LinuxUpdater::~LinuxUpdater() {
            CancelUpdate();
        }

        bool LinuxUpdater::Initialize(const UpdateConfig& config) {
            config_ = config;

            // 确保临时目录存在
            if (config_.temp_dir.empty()) {
                config_.temp_dir =
                    QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString() + "/GDownloadUpdater";
            }

            QDir dir(QString::fromStdString(config_.temp_dir));
            if (!dir.exists() && !dir.mkpath(".")) {
                last_error_ = "无法创建临时目录: " + config_.temp_dir;
                return false;
            }

            return true;
        }

        void LinuxUpdater::CheckForUpdates(UpdateCheckCallback callback) {
            if (update_in_progress_) {
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }

            // 获取当前AppImage路径
            QString appimage_path = QCoreApplication::applicationFilePath();

            // 如果是AppImage，使用libappimageupdate检查更新
            try {
                // 创建updater实例
                if (!updater_){
                    updater_ = std::make_unique<appimage::update::Updater>(appimage_path.toStdString());
                }

                // 检查更新
                bool update_available = false;
                if (!updater_->checkForChanges(update_available)) {
                    // 记录错误信息
                    std::string message;
                    while (updater_->nextStatusMessage(message)) {
                        last_error_ += message + "\n";
                    }

                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }

                update_available_ = update_available;

                if (update_available) {
                    // 填充更新信息
                    update_info_.version	   = "新版本";	// AppImageUpdate不提供版本信息
                    update_info_.download_url  = "";		// 由AppImageUpdate内部处理
                    update_info_.release_notes = "";		// AppImageUpdate不提供发布说明
                    update_info_.is_mandatory  = false;

                    // 获取更多信息
                    std::string message;
                    while (updater_->nextStatusMessage(message)) {
                        update_info_.release_notes += message + "\n";
                    }
                }

                if (callback) {
                    callback(update_available, update_info_);
                }
            } catch (const std::exception& e) {
                last_error_ = std::string("检查更新失败: ") + e.what();
                if (callback) {
                    callback(false, UpdateInfo{});
                }
            }
        }

        bool LinuxUpdater::StartUpdate(ProgressCallback progress_callback) {
            if (!update_available_ || update_in_progress_) {
                last_error_ = "没有可用更新或更新已在进行中";
                return false;
            }

            progress_callback_	= progress_callback;
            update_in_progress_ = true;
            should_stop_thread_ = false;

            // 获取当前AppImage路径
            QString appimage_path = QCoreApplication::applicationFilePath();

            // 检查是否是AppImage
            if (!appimage_path.contains(".AppImage")) {
                last_error_			= "当前应用不是AppImage格式，无法使用AppImageUpdate更新";
                update_in_progress_ = false;
                return false;
            }

            try {
                // 如果updater_为空，创建一个新实例
                if (!updater_) {
                    updater_ = std::make_unique<appimage::update::Updater>(appimage_path.toStdString());
                }

                // 启动更新线程
                update_thread_ = std::thread([this]() {
                    // 启动更新
                    updater_->start();

                    // 监控更新进度
                    UpdateProgressThread();
                });

                return true;
            } catch (const std::exception& e) {
                last_error_			= std::string("启动更新失败: ") + e.what();
                update_in_progress_ = false;
                return false;
            }
        }

        void LinuxUpdater::UpdateProgressThread() {
            // 监控更新进度，直到完成或被取消
            while (!updater_->isDone() && !should_stop_thread_) {
                // 避免CPU占用过高
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // 获取进度
                double progress = 0.0;
                if (updater_->progress(progress)) {
                    if (progress_callback_) {
                        UpdateProgress update_progress;

                        // 根据进度确定阶段
                        if (progress < 0.5) {
                            update_progress.stage = UpdateProgress::Stage::kDownloading;
                        }
                        else if (progress < 0.75) {
                            update_progress.stage = UpdateProgress::Stage::kExtracting;
                        }
                        else if (progress < 0.9) {
                            update_progress.stage = UpdateProgress::Stage::kVerifying;
                        }
                        else {
                            update_progress.stage = UpdateProgress::Stage::kInstalling;
                        }

                        update_progress.percentage = static_cast<int>(progress * 100);

                        // 获取状态消息
                        std::string message;
                        if (updater_->nextStatusMessage(message)) {
                            update_progress.message = message;
                        }
                        else {
                            // 如果没有新消息，使用默认消息
                            switch (update_progress.stage) {
                                case UpdateProgress::Stage::kDownloading:
                                    update_progress.message = "正在下载更新...";
                                    break;
                                case UpdateProgress::Stage::kExtracting:
                                    update_progress.message = "正在解压更新...";
                                    break;
                                case UpdateProgress::Stage::kVerifying:
                                    update_progress.message = "正在验证更新...";
                                    break;
                                case UpdateProgress::Stage::kInstalling:
                                    update_progress.message = "正在安装更新...";
                                    break;
                                default:
                                    update_progress.message = "正在更新...";
                                    break;
                            }
                        }

                        progress_callback_(update_progress);
                    }
                }

                // 记录所有消息
                LogMessages();
            }

            // 更新完成，检查是否有错误
            if (!should_stop_thread_) {
                if (updater_->hasError()) {
                    if (progress_callback_) {
                        UpdateProgress progress;
                        progress.stage		= UpdateProgress::Stage::kFailed;
                        progress.percentage = 0;

                        // 获取错误消息
                        std::string message;
                        if (updater_->nextStatusMessage(message)) {
                            progress.message = "更新失败: " + message;
                        }
                        else {
                            progress.message = "更新失败";
                        }

                        progress_callback_(progress);
                    }
                }
                else {
                    if (progress_callback_) {
                        UpdateProgress progress;
                        progress.stage		= UpdateProgress::Stage::kFinished;
                        progress.percentage = 100;
                        progress.message	= "更新完成，准备应用更新";
                        progress_callback_(progress);
                    }
                }
            }

            update_in_progress_ = false;
        }

        void LinuxUpdater::LogMessages() {
            // 记录所有消息
            std::string message;
            while (updater_->nextStatusMessage(message)) {
                // 在实际应用中，可能需要将这些消息记录到日志文件
                // 这里简单地将它们添加到last_error_
                if (!message.empty()) {
                    last_error_ += message + "\n";
                }
            }
        }

        void LinuxUpdater::CancelUpdate() {
            if (current_reply_) {
                current_reply_->abort();
                current_reply_->deleteLater();
                current_reply_ = nullptr;
            }

            // 标记线程应该停止
            should_stop_thread_ = true;

            // 等待线程结束
            if (update_thread_.joinable()) {
                update_thread_.join();
            }

            update_in_progress_ = false;
        }

        bool LinuxUpdater::ApplyUpdate(bool restart_app) {
            if (!updater_ || updater_->hasError()) {
                last_error_ = "没有准备好的更新包或更新过程中出现错误";
                return false;
            }

            // 获取新文件路径
            std::string new_file_path;
            if (!updater_->pathToNewFile(new_file_path) || new_file_path.empty()) {
                last_error_ = "无法获取更新后的AppImage路径";
                return false;
            }

            if (progress_callback_) {
                UpdateProgress progress;
                progress.stage		= UpdateProgress::Stage::kInstalling;
                progress.percentage = 95;
                progress.message	= "准备应用更新...";
                progress_callback_(progress);
            }

            // 确保新文件存在
            if (!QFile::exists(QString::fromStdString(new_file_path))) {
                last_error_ = "更新后的AppImage文件不存在";
                return false;
            }

            // 设置可执行权限
            QFile file(QString::fromStdString(new_file_path));
            file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup |
                                QFile::ExeOther);

            if (restart_app) {
                // 启动新版本
                QProcess::startDetached(QString::fromStdString(new_file_path), QStringList());

                // 退出当前应用
                QCoreApplication::quit();
            }

            if (progress_callback_) {
                UpdateProgress progress;
                progress.stage		= UpdateProgress::Stage::kFinished;
                progress.percentage = 100;
                progress.message	= "更新完成";
                progress_callback_(progress);
            }

            return true;
        }

    }  // namespace update
}  // namespace gdl
#endif
