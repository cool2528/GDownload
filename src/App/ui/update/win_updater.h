#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <atomic>
#include <memory>
#include "auto_updater.h"

namespace gdl {
    namespace update {

        class WinUpdater : public AutoUpdater {
           public:
            WinUpdater();
            ~WinUpdater() override;

            bool Initialize(const UpdateConfig& config) override;
            void CheckForUpdates(UpdateCheckCallback callback) override;
            bool StartUpdate(ProgressCallback progress_callback) override;
            void CancelUpdate() override;
            bool ApplyUpdate(bool restart_app = true) override;

           private:
            void startCheckRequest(const std::string& update_url, bool is_fallback, UpdateCheckCallback callback);
            void handleNetworkReply(QNetworkReply* reply, bool is_fallback, UpdateCheckCallback callback);

            UpdateConfig config_;
            UpdateInfo update_info_;
            ProgressCallback progress_callback_;
            QNetworkAccessManager network_manager_;
            QFile download_file_;
            QString update_package_path_;
            QNetworkReply* current_check_reply_ = nullptr;
            // 当前下载请求，供 CancelUpdate 中止；errorOccurred 与 finished 均会清理。
            QNetworkReply* current_download_reply_ = nullptr;
            std::shared_ptr<std::atomic<bool>> alive_{std::make_shared<std::atomic<bool>>(true)};
            // 防止 errorOccurred 与 finished 重复触发失败通知。
            std::atomic<bool> update_available_{false};
            std::atomic<bool> update_in_progress_{false};
            std::atomic<bool> download_failed_notified_{false};
        };

    }  // namespace update
}  // namespace gdl
