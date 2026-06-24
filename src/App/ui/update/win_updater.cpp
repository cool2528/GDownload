#include "win_updater.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <nlohmann/json.hpp>
#include "config/config.h"
namespace gdl {
    namespace update {

        namespace {
            constexpr const char* kGithubMirrorPrefix = "https://gh-proxy.com/";

            std::string NormalizeReleaseNotes(std::string raw) {
                auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
                    size_t pos = 0;
                    while ((pos = s.find(from, pos)) != std::string::npos) {
                        s.replace(pos, from.length(), to);
                        pos += to.length();
                    }
                };
                replace_all(raw, "\r\n", "\n");
                replace_all(raw, "\r", "\n");
                replace_all(raw, "<br>", "\n");
                replace_all(raw, "<br/>", "\n");
                replace_all(raw, "<br />", "\n");
                return raw;
            }

            std::string ApplyGithubMirrorIfNeeded(const std::string& original_url) {
                if (original_url.empty()) {
                    return original_url;
                }

                const bool enable_mirror =
                    gdl::config::GetValue(std::string(config::Keys::EnableGithubAccelerate.get())).AsBool();
                if (!enable_mirror) {
                    return original_url;
                }

                if (original_url.rfind(kGithubMirrorPrefix, 0) == 0) {
                    return original_url;
                }

                const bool is_github_resource =
                    original_url.find("github.com") != std::string::npos ||
                    original_url.find("githubusercontent.com") != std::string::npos;
                if (!is_github_resource) {
                    return original_url;
                }

                return std::string(kGithubMirrorPrefix) + original_url;
            }
        }  // namespace

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
            alive_->store(false);
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

            startCheckRequest(config_.update_url, false, callback);
        }

        void WinUpdater::startCheckRequest(const std::string& update_url, bool is_fallback,
                                           UpdateCheckCallback callback) {
            if (update_url.empty()) {
                last_error_ = is_fallback ? "Fallback update URL is empty" : "Update URL is empty";
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }

            QNetworkRequest request(QUrl(QString::fromStdString(update_url)));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            for (const auto& header : request_headers_) {
                request.setRawHeader(header.first.c_str(), header.second.c_str());
            }
            auto reply = network_manager_.get(request);
            if (!reply) {
                last_error_ = "Failed to create network request";
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }
            current_check_reply_ = reply;
            QObject::connect(reply, &QNetworkReply::finished, [this, reply, is_fallback, callback, alive = alive_]() {
                if (!alive->load()) {
                    reply->deleteLater();
                    return;
                }
                this->handleNetworkReply(reply, is_fallback, callback);
                if (current_check_reply_ == reply) {
                    current_check_reply_ = nullptr;
                }
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
                    progress.stage		= UpdateProgress::Stage::kInstalling;
                    progress.percentage = 100;
                    progress.message	= "Using already downloaded update package";
                    progress_callback_(progress);
                }
                return true;
            }

            // Start download
            const auto actual_download_url = ApplyGithubMirrorIfNeeded(update_info_.download_url);
            QNetworkRequest request(QUrl(QString::fromStdString(actual_download_url)));

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
            current_download_reply_	   = reply;  // 供 CancelUpdate 中止
            download_failed_notified_ = false;    // 重置失败通知标志

            // Connect signals
			QObject::connect(reply, &QNetworkReply::readyRead, [this, reply, alive = alive_]() {
                if (!alive->load()) return;
                if (download_file_.isOpen() && reply) {
                    download_file_.write(reply->readAll());
                }
            });

            QObject::connect(
                reply, &QNetworkReply::downloadProgress,
				[this, downloaded_bytes, reply, alive = alive_](qint64 bytesReceived, qint64 bytesTotal) {
                    if (!alive->load()) return;
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
			QObject::connect(reply, &QNetworkReply::errorOccurred, [this, reply, alive = alive_](QNetworkReply::NetworkError error) {
                if (!alive->load()) {
                    reply->deleteLater();
                    return;
                }
                last_error_			= reply->errorString().toStdString();
                update_in_progress_ = false;
                download_file_.close();
                if (current_download_reply_ == reply) {
                    current_download_reply_ = nullptr;
                }
                // finished 也会触发，用标志避免重复的失败通知。
                bool expected = false;
                if (download_failed_notified_.compare_exchange_strong(expected, true) && progress_callback_) {
					UpdateProgress progress;
					progress.stage		= UpdateProgress::Stage::kFailed;
					progress.percentage = 0;
					progress.message	= "Download failed: " + last_error_;
					progress_callback_(progress);
				}

				reply->deleteLater();
				return;
            });
            QObject::connect(reply, &QNetworkReply::finished, [this, reply, alive = alive_]() {
                if (!alive->load()) {
                    reply->deleteLater();
                    return;
                }
                if (download_file_.isOpen()) {
                    download_file_.close();
                }
                if (current_download_reply_ == reply) {
                    current_download_reply_ = nullptr;
                }

                if (reply->error() != QNetworkReply::NoError) {
                    last_error_			= reply->errorString().toStdString();
                    update_in_progress_ = false;

                    bool expected = false;
                    if (download_failed_notified_.compare_exchange_strong(expected, true) && progress_callback_) {
                        UpdateProgress progress;
                        progress.stage		= UpdateProgress::Stage::kFailed;
                        progress.percentage = 0;
                        progress.message	= "Download failed: " + last_error_;
                        progress_callback_(progress);
                    }

                    reply->deleteLater();
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
                    return;
                }

                // Download complete
                if (progress_callback_) {
                    UpdateProgress progress;
					progress.stage		= UpdateProgress::Stage::kInstalling;
                    progress.percentage = 100;
					progress.message = "Download complete, ready to start updating";
                    progress_callback_(progress);
                }

                reply->deleteLater();
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
            if (current_check_reply_) {
                current_check_reply_->abort();
                current_check_reply_ = nullptr;
            }
            // 中止正在进行的下载请求，否则后台仍会下载并触发回调访问 this。
            if (current_download_reply_) {
                current_download_reply_->abort();
                current_download_reply_ = nullptr;
            }

            if (download_file_.isOpen()) {
                download_file_.close();
            }

            update_in_progress_ = false;
        }

        bool WinUpdater::ApplyUpdate(bool restart_app) {

            // Check if update package exists
            QFileInfo file_info(update_package_path_);
            if (!file_info.exists() || !file_info.isFile()) {
                last_error_ = "Update package file does not exist: " + update_package_path_.toStdString();
                update_in_progress_ = false;
                return false;
            }

            if (progress_callback_) {
                UpdateProgress progress;
                progress.stage		= UpdateProgress::Stage::kInstalling;
                progress.percentage = 0;
                progress.message	= "Launching installer...";
                progress_callback_(progress);
            }

            // Launch the installer
			QProcess installer;
			installer.setProgram(update_package_path_);
			//installer.setArguments(QStringList() << "/S");	// Silent install
			installer.setProcessChannelMode(QProcess::MergedChannels);
			installer.start();
			UpdateProgress progress;
			if (!installer.waitForStarted()) {
				last_error_ = "Failed to start installer: " + update_package_path_.toStdString();
				progress.stage = UpdateProgress::Stage::kFailed;
				progress.message = last_error_;
				if (progress_callback_) {
					progress.percentage = 100;
					progress_callback_(progress);
				}
				update_in_progress_ = false;
				return false;
			}
			if (!installer.waitForFinished()) {
				last_error_ = "Installer did not finish: " + update_package_path_.toStdString();
				progress.stage = UpdateProgress::Stage::kFailed;
				progress.message = last_error_;
				if (progress_callback_) {
					progress.percentage = 100;
					progress_callback_(progress);
				}
				update_in_progress_ = false;
				return false;
			}
			if (installer.exitCode() != 0) {
				last_error_ = "Installer failed with exit code: " + std::to_string(installer.exitCode());
				progress.stage = UpdateProgress::Stage::kFailed;
				progress.message = last_error_;
				if (progress_callback_) {
					progress.percentage = 100;
					progress_callback_(progress);
				}
				update_in_progress_ = false;
				return false;
			}
			if (progress_callback_) {
				progress.percentage = 100;
				progress.stage		= UpdateProgress::Stage::kFinished;
				progress.message	= "Update installed successfully";
				progress_callback_(progress);
			}
            update_in_progress_ = false;
            return true;
        }

        void WinUpdater::handleNetworkReply(QNetworkReply* reply, bool is_fallback, UpdateCheckCallback callback) {
            if (!reply) {
                last_error_ = "Network reply is nullptr";
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;  // 必须返回，否则下方 reply->error() 空指针解引用崩溃
            }
            auto error = reply->error();
            if (error != QNetworkReply::NoError) {
                // 用户主动取消检查请求（CancelUpdate 调用 abort）时不应触发 fallback，
                // 保持取消语义：直接回调失败，不切换到备用更新源。
                if (error == QNetworkReply::OperationCanceledError) {
                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }
                last_error_ = reply->errorString().toStdString();
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
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
                    if (!is_fallback && !config_.fallback_update_url.empty()) {
                        startCheckRequest(config_.fallback_update_url, true, callback);
                        return;
                    }
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
                info.release_notes = NormalizeReleaseNotes(doc["body"].get<std::string>());
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
                        // 成功找到可更新的安装包，清空可能残留的上次检查错误信息
                        last_error_.clear();
                        update_available_ = true;
                        update_info_	  = info;
                        if (callback) {
                            callback(true, info);
                        }
                        return;
                    }
                }
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    last_error_ = "No Windows update package found in primary update info";
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }

            } catch (std::exception& e) {
                last_error_ = e.what();
                if (!is_fallback && !config_.fallback_update_url.empty()) {
                    startCheckRequest(config_.fallback_update_url, true, callback);
                    return;
                }
                if (callback) {
                    callback(false, UpdateInfo{});
                }
                return;
            }
        }

    }  // namespace update
}  // namespace gdl
