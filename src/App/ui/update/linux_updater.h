#pragma once
#if defined(__linux__)
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>
#include <thread>
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

            UpdateConfig config_;
            UpdateInfo update_info_;
            ProgressCallback progress_callback_;
            UpdateCheckCallback check_callback_;
            QNetworkAccessManager network_manager_;
            QNetworkReply* current_reply_ = nullptr;

            // libappimageupdate实例
            std::unique_ptr<appimage::update::Updater> updater_;
            std::thread update_thread_;
            bool update_available_	 = false;
            bool update_in_progress_ = false;
            bool should_stop_thread_ = false;
        };

    }  // namespace update
}  // namespace gdl

#endif
