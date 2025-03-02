#include "win_updater.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <nlohmann/json.hpp>
namespace gdl {
    namespace update {

        namespace VersionTools {
            class Version {
               public:
                Version() = default;
                explicit Version(const std::string& version_str) { *this = StringToVersion(version_str); }

                explicit Version(int major, int minor, int patch, const std::string& build)
                    : major_(major), minor_(minor), patch_(patch), build_(build) {}

                Version(const Version& other)
                    : major_(other.major_), minor_(other.minor_), patch_(other.patch_), build_(other.build_) {}

                Version& operator=(const Version& other) {
                    if (this != &other) {
                        major_ = other.major_;
                        minor_ = other.minor_;
                        patch_ = other.patch_;
                        build_ = other.build_;
                    }
                    return *this;
                }

                bool operator<(const Version& other) const {
                    if (major_ != other.major_) return major_ < other.major_;
                    if (minor_ != other.minor_) return minor_ < other.minor_;
                    if (patch_ != other.patch_) return patch_ < other.patch_;
                    return false;
                }

                bool operator==(const Version& other) const {
                    return major_ == other.major_ && minor_ == other.minor_ && patch_ == other.patch_;
                }

                bool operator>(const Version& other) const { return other < *this; }

                bool operator<=(const Version& other) const { return *this < other || *this == other; }

                bool operator>=(const Version& other) const { return *this > other || *this == other; }

                bool IsValid() const { return major_ != 0 || minor_ != 0 || patch_ != 0; }

                std::string ToString() const {
                    return std::to_string(major_) + "." + std::to_string(minor_) + "." + std::to_string(patch_) + "." +
                           build_;
                }

               private:
                Version StringToVersion(const std::string& version_str) {
                    if (version_str.empty()) {
                        return Version();
                    }

                    std::vector<std::string> parts;
                    std::stringstream ss(version_str);
                    std::string item;

                    while (std::getline(ss, item, '.')) {
                        parts.push_back(item);
                    }

                    if (parts.size() < 3) {
                        return Version();
                    }

                    for (size_t i = 0; i < 3; i++) {
                        for (char c : parts[i]) {
                            if (!std::isdigit(c)) {
                                return Version();
                            }
                        }
                    }
                    int major		  = std::stoi(parts[0]);
                    int minor		  = std::stoi(parts[1]);
                    int patch		  = std::stoi(parts[2]);
                    std::string build = parts.size() > 3 ? parts[3] : "";

                    return Version(major, minor, patch, build);
                }

               private:
                int major_{0};
                int minor_{0};
                int patch_{0};
                std::string build_;
            };
        }  // namespace VersionTools
        WinUpdater::WinUpdater() = default;

        WinUpdater::~WinUpdater() {
            CancelUpdate();
        }

        bool WinUpdater::Initialize(const UpdateConfig& config) {
            config_ = config;

            // Ensure temp directory exists
            if (config_.temp_dir.empty()) {
                config_.temp_dir =
                    QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString() + "/GDownloadUpdater";
            }

            QDir dir(QString::fromStdString(config_.temp_dir));
            if (!dir.exists() && !dir.mkpath(".")) {
                last_error_ = "Failed to create temp directory: " + config_.temp_dir;
                return false;
            }

            return true;
        }

        void WinUpdater::CheckForUpdates(UpdateCheckCallback callback) {
            if (update_in_progress_) {
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }
            QNetworkRequest request(QUrl(QString::fromStdString(config_.update_url)));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            for (const auto& header : request_headers_) {
                request.setRawHeader(header.first.c_str(), header.second.c_str());
            }
            auto reply = network_manager_.get(request);
            if (!reply) {
                last_error_ = "Failed to create network request";
                return;
            }
            QObject::connect(reply, &QNetworkReply::finished, [this, reply, callback]() {
                this->handleNetworkReply(reply, callback);
                reply->deleteLater();
            });
        }

        bool WinUpdater::StartUpdate(ProgressCallback progress_callback) {
            if (!update_available_ || update_in_progress_) {
                last_error_ = "No available updates or update already in progress";
                return false;
            }

            progress_callback_	= progress_callback;
            update_in_progress_ = true;

            // Create download path
            update_package_path_ = QString::fromStdString(config_.temp_dir) + "/gdownload_latest_" +
                                   QString::fromStdString(update_info_.version) + ".exe";

            // Check if the same version package already exists
            QFileInfo file_info(update_package_path_);
            if (file_info.exists() && file_info.size() == update_info_.package_size) {
                // File exists and size matches, skip download
                if (progress_callback_) {
                    UpdateProgress progress;
                    progress.stage		= UpdateProgress::Stage::kFinished;
                    progress.percentage = 100;
                    progress.message	= "Using already downloaded update package";
                    progress_callback_(progress);
                }
                return true;
            }

            // Start download
            QNetworkRequest request(QUrl(QString::fromStdString(update_info_.download_url)));

            // Support resume download
            if (file_info.exists() && file_info.size() > 0 && file_info.size() < update_info_.package_size) {
                QString range_header = QString("bytes=%1-").arg(file_info.size());
                request.setRawHeader("Range", range_header.toUtf8());
            }

            // Add custom request headers
            for (const auto& header : request_headers_) {
                request.setRawHeader(header.first.c_str(), header.second.c_str());
            }

            // Set timeout
            request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

            // Open file for writing
            QIODevice::OpenMode open_mode = QIODevice::WriteOnly;
            if (request.hasRawHeader("Range")) {
                open_mode |= QIODevice::Append;
            }

            download_file_.setFileName(update_package_path_);
            if (!download_file_.open(open_mode)) {
                last_error_			= "Failed to create download file: " + update_package_path_.toStdString();
                update_in_progress_ = false;
                return false;
            }

            // Record downloaded bytes (for resume)
            qint64 downloaded_bytes = request.hasRawHeader("Range") ? file_info.size() : 0;

            // Create and send request
            auto reply = network_manager_.get(request);
            if (!reply) {
                last_error_			= "Failed to create network request";
                update_in_progress_ = false;
                download_file_.close();
                return false;
            }

            // Connect signals
            QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
                if (download_file_.isOpen() && reply) {
                    download_file_.write(reply->readAll());
                }
            });

            QObject::connect(
                reply, &QNetworkReply::downloadProgress,
                [this, downloaded_bytes](qint64 bytesReceived, qint64 bytesTotal) {
                    if (!progress_callback_) return;

                    // Calculate actual progress (considering resume)
                    qint64 actual_received = bytesReceived + downloaded_bytes;
                    qint64 actual_total	   = bytesTotal > 0 ? bytesTotal + downloaded_bytes : update_info_.package_size;

                    int percentage = actual_total > 0 ? static_cast<int>(actual_received * 100 / actual_total) : 0;

                    UpdateProgress progress;
                    progress.stage		= UpdateProgress::Stage::kDownloading;
                    progress.percentage = percentage;
                    progress.message	= "Downloading update package: " + std::to_string(actual_received / 1024) +
                                       "KB / " + std::to_string(actual_total / 1024) + "KB";
                    progress_callback_(progress);
                });

            QObject::connect(reply, &QNetworkReply::finished, [&]() {
                if (download_file_.isOpen()) {
                    download_file_.close();
                }

                if (reply->error() != QNetworkReply::NoError) {
                    last_error_			= reply->errorString().toStdString();
                    update_in_progress_ = false;

                    if (progress_callback_) {
                        UpdateProgress progress;
                        progress.stage		= UpdateProgress::Stage::kFailed;
                        progress.percentage = 0;
                        progress.message	= "Download failed: " + last_error_;
                        progress_callback_(progress);
                    }

                    reply->deleteLater();
                    reply = nullptr;
                    return;
                }

                // Verify file size
                QFileInfo file_info(update_package_path_);
                if (file_info.size() != update_info_.package_size) {
                    last_error_			= "Downloaded file size mismatch";
                    update_in_progress_ = false;

                    if (progress_callback_) {
                        UpdateProgress progress;
                        progress.stage		= UpdateProgress::Stage::kFailed;
                        progress.percentage = 0;
                        progress.message	= "File size verification failed";
                        progress_callback_(progress);
                    }

                    reply->deleteLater();
                    reply = nullptr;
                    return;
                }

                // Download complete
                if (progress_callback_) {
                    UpdateProgress progress;
                    progress.stage		= UpdateProgress::Stage::kFinished;
                    progress.percentage = 100;
                    progress.message	= "Update package download complete";
                    progress_callback_(progress);
                }

                reply->deleteLater();
                reply = nullptr;
            });

            // Report download start
            if (progress_callback_) {
                UpdateProgress progress;
                progress.stage		= UpdateProgress::Stage::kDownloading;
                progress.percentage = 0;
                progress.message	= "Starting update package download";
                progress_callback_(progress);
            }

            return true;
        }

        void WinUpdater::CancelUpdate() {

            if (download_file_.isOpen()) {
                download_file_.close();
            }

            update_in_progress_ = false;
        }

        bool WinUpdater::ApplyUpdate(bool restart_app) {
            if (!update_in_progress_) {
                last_error_ = "No update package ready";
                return false;
            }

            // Check if update package exists
            QFileInfo file_info(update_package_path_);
            if (!file_info.exists() || !file_info.isFile()) {
                last_error_ = "Update package file does not exist: " + update_package_path_.toStdString();
                return false;
            }

            if (progress_callback_) {
                UpdateProgress progress;
                progress.stage		= UpdateProgress::Stage::kInstalling;
                progress.percentage = 0;
                progress.message	= "Launching installer...";
                progress_callback_(progress);
            }

            // Build installer command line arguments
            QStringList arguments;
            arguments << "/SILENT";	 // Silent install, only show progress bar

            if (restart_app) {
                arguments << "/RESTARTAPPLICATIONS";  // Restart application after install
            }
            else {
                arguments << "/NORESTART";	// Don't restart application
            }

            // Launch installer

            return true;
        }

        void WinUpdater::handleNetworkReply(QNetworkReply* reply, UpdateCheckCallback callback) {
            if (!reply) {
                last_error_ = "Network reply is nullptr";
                if (callback) {
                    callback(false, UpdateInfo{});
                }
            }
            auto error = reply->error();
            if (error != QNetworkReply::NoError) {
                last_error_ = reply->errorString().toStdString();
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }

            QByteArray data = reply->readAll();
            try {

                nlohmann::json doc = nlohmann::json::parse(data.toStdString());
                UpdateInfo info;
                if (!doc.contains("tag_name")) {
                    last_error_ = "Invalid update info";
                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }
                QString tag_name = QString::fromStdString(doc["tag_name"].get<std::string>());
                tag_name		 = tag_name.replace("v", "");
                VersionTools::Version new_version(tag_name.toStdString());
                VersionTools::Version old_version(config_.current_version);
                if (new_version <= old_version) {
                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }
                info.version		 = tag_name.toStdString();
                QString published_at = QString::fromStdString(doc["published_at"].get<std::string>());
                info.release_date =
                    QDateTime::fromString(published_at, Qt::ISODate).toString("yyyy-MM-dd hh:mm:ss").toStdString();
                info.release_notes = doc["body"].get<std::string>();
                if (doc.contains("assets") && doc["assets"].is_array()) {
                    for (auto asset : doc["assets"]) {
                        if (asset.contains("browser_download_url") && asset.contains("name")) {
                            const auto name = asset["name"].get<std::string>();
                            if (name.find(".exe") == std::string::npos) {
                                continue;
                            }
                            info.download_url = asset["browser_download_url"].get<std::string>();
                            info.package_size = asset["size"].get<int64_t>();
                            break;
                        }
                    }
                    if (!info.download_url.empty()) {
                        update_available_ = true;
                        update_info_	  = info;
                        if (callback) {
                            callback(true, info);
                        }
                        return;
                    }
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }

            } catch (std::exception& e) {
                last_error_ = e.what();
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }
        }

    }  // namespace update
}  // namespace gdl
