#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
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
            void handleNetworkReply(QNetworkReply* reply, UpdateCheckCallback callback);

            UpdateConfig config_;
            UpdateInfo update_info_;
            ProgressCallback progress_callback_;
            QNetworkAccessManager network_manager_;
            QFile download_file_;
            QString update_package_path_;
            bool update_available_	 = false;
            bool update_in_progress_ = false;
        };

    }  // namespace update
}  // namespace gdl
