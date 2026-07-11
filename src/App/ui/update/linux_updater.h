#pragma once
#if defined(__linux__)
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include "auto_updater.h"
namespace appimage {
    namespace update {
        class Updater;
    }
}  // namespace appimage

namespace gdl {
    namespace update {

        class LinuxUpdater : public AutoUpdater {
           public:
            LinuxUpdater();
            ~LinuxUpdater() override;

            bool Initialize(const UpdateConfig& config) override;
            void CheckForUpdates(UpdateCheckCallback callback) override;
            bool StartUpdate(ProgressCallback progress_callback) override;
            void CancelUpdate() override;
            bool ApplyUpdate(bool restart_app = true) override;

           private:
            void UpdateProgressThread();
            void LogMessages();
			void DispatchProgress(UpdateProgress progress);
            void startCheckRequest(const std::string& update_url, bool is_fallback, UpdateCheckCallback callback);
            void handleNetworkReply(QNetworkReply* reply, bool is_fallback, UpdateCheckCallback callback);
            UpdateConfig config_;
            UpdateInfo update_info_;
            ProgressCallback progress_callback_;
            UpdateCheckCallback check_callback_;
            QNetworkAccessManager network_manager_;
            QNetworkReply* current_reply_ = nullptr;
            std::shared_ptr<std::atomic<bool>> alive_{std::make_shared<std::atomic<bool>>(true)};

            // libappimageupdate instance
            std::unique_ptr<appimage::update::Updater> updater_;
            std::thread update_thread_;
            // 跨线程访问（UI 线程写、update 线程读），必须为原子以避免数据竞争。
            std::atomic<bool> update_available_{false};
            std::atomic<bool> update_in_progress_{false};
            std::atomic<bool> should_stop_thread_{false};
			std::atomic<bool> update_worker_running_{false};
			std::mutex update_thread_mutex_;
        };

    }  // namespace update
}  // namespace gdl

#endif
