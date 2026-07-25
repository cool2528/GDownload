#include "win_updater.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QLockFile>
#include <nlohmann/json.hpp>
#include "config/config.h"
#include <QFile>
#include "logger.h"
#include "update/update_manifest.h"
#include "update/update_package_verifier.h"
#include "update/update_url_policy.h"
#include "update/redirect_chain_controller.h"
#include "update_trust_config.h"
#include "file_update_rollback_store.h"
#ifdef _WIN32
#include <windows.h>
#endif
namespace gdl {
    namespace update {

        namespace {
#ifdef _WIN32
			class LockedUpdatePackage final {
			 public:
				explicit LockedUpdatePackage(const std::string& path) {
					const auto wide = QString::fromStdString(path).toStdWString();
					handle_ = CreateFileW(wide.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				}
				~LockedUpdatePackage() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
				bool IsValid() const { return handle_ != INVALID_HANDLE_VALUE; }
			 private:
				HANDLE handle_{INVALID_HANDLE_VALUE};
			};
#endif
			class QProcessInstallerLauncher final : public IInstallerLauncher {
			 public:
				InstallationResult LaunchAndWait(const std::filesystem::path& package, std::stop_token stop_token) override {
					QProcess process;
					process.setProgram(QString::fromStdString(package.string()));
					process.start();
					if (!process.waitForStarted(10000))
						return {InstallationStatus::kLaunchFailed, 0, process.errorString().toStdString()};
					const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(30);
					while (!process.waitForFinished(200)) {
						if (stop_token.stop_requested()) {
							process.terminate();
							if (!process.waitForFinished(2000)) { process.kill(); process.waitForFinished(2000); }
							return {InstallationStatus::kCancelledOrNonZero, 0, "installer cancelled"};
						}
						if (std::chrono::steady_clock::now() >= deadline) {
							process.kill(); process.waitForFinished(5000);
							return {InstallationStatus::kTimedOut, 0, "installer timed out"};
						}
					}
					if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
						return {InstallationStatus::kCancelledOrNonZero, process.exitCode(), "installer failed"};
					return {InstallationStatus::kSucceeded, 0, {}};
				}
			};
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
            // 流式计算文件 SHA-256，返回小写十六进制；失败返回空（S2）
            QString ComputeFileSha256(const QString& path) {
                QFile f(path);
                if (!f.open(QIODevice::ReadOnly)) return {};
                QCryptographicHash hash(QCryptographicHash::Sha256);
                if (!hash.addData(&f)) return {};
                return QString::fromLatin1(hash.result().toHex());
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
                    return BuildNumber() < other.BuildNumber();  // 纳入第 4 段 build 号比较（B3）
                }

                bool operator==(const Version& other) const {
                    return major_ == other.major_ && minor_ == other.minor_ && patch_ == other.patch_ &&
                           BuildNumber() == other.BuildNumber();
                }

                bool operator>(const Version& other) const { return other < *this; }

                bool operator<=(const Version& other) const { return *this < other || *this == other; }

                bool operator>=(const Version& other) const { return *this > other || *this == other; }

                // build 段可能非数字，数值化失败按 0 处理（B3）
                long long BuildNumber() const {
                    try {
                        return build_.empty() ? 0 : std::stoll(build_);
                    } catch (...) {
                        return 0;
                    }
                }

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
        WinUpdater::WinUpdater()
			: WinUpdater(CreatePlatformPackageVerifier(), std::make_unique<QProcessInstallerLauncher>(),
				std::make_unique<FileUpdateRollbackStore>(std::filesystem::path(
					QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()) /
					"update" / "highest_release_id")) {}

		WinUpdater::WinUpdater(std::unique_ptr<IPlatformPackageVerifier> verifier,
			std::unique_ptr<IInstallerLauncher> launcher,
			std::unique_ptr<IUpdateRollbackStore> rollback_store)
			: platform_verifier_(std::move(verifier)), installer_launcher_(std::move(launcher)),
			  rollback_store_(std::move(rollback_store)) {}

        WinUpdater::~WinUpdater() {
            alive_->store(false);
            CancelUpdate();
			if (installation_thread_.joinable()) installation_thread_.join();
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

            update_package_path_ = QString::fromStdString(CreateUniqueUpdateTempPath(config_.temp_dir, ".exe").string());
            QFileInfo file_info(update_package_path_);

            // Start download
			const std::vector<std::string> allowed_download_hosts{"github.com", "objects.githubusercontent.com", "gdownload.uk"};
			const auto actual_download_url = update_info_.download_url;
			if (!ValidateDownloadUrl(actual_download_url, allowed_download_hosts)) {
				last_error_ = "Update download URL is not allowed";
				update_in_progress_ = false;
				return false;
			}
            QNetworkRequest request(QUrl(QString::fromStdString(actual_download_url)));


            // Add custom request headers
            for (const auto& header : request_headers_) {
                request.setRawHeader(header.first.c_str(), header.second.c_str());
            }

            // Set timeout
            request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);

            // Open file for writing
            QIODevice::OpenMode open_mode = QIODevice::WriteOnly | QIODevice::Truncate;

            download_file_.setFileName(update_package_path_);
            if (!download_file_.open(open_mode)) {
                last_error_			= "Failed to create download file: " + update_package_path_.toStdString();
                update_in_progress_ = false;
                return false;
            }

            // Record downloaded bytes (for resume)
            qint64 downloaded_bytes = 0;

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
			auto redirect_chain = std::make_shared<RedirectChainController>(actual_download_url,
				allowed_download_hosts, 5);
			QObject::connect(reply, &QNetworkReply::redirected, [reply, redirect_chain](const QUrl& target) {
				const auto result = redirect_chain->Follow(target.toString().toStdString());
				if (result.decision == RedirectChainDecision::kFollow) reply->redirectAllowed();
				else reply->abort();
			});

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

                const auto verification = VerifyUpdatePackage(update_package_path_.toStdString(),
                    update_info_.package_size, update_info_.sha256);
                if (!verification.ok) {
                        QFile::remove(update_package_path_);
                        last_error_ = verification.error;
                        update_in_progress_ = false;
                        if (progress_callback_) {
                            UpdateProgress progress;
                            progress.stage = UpdateProgress::Stage::kFailed;
                            progress.percentage = 0;
                            progress.message = "Update package verification failed";
                            progress_callback_(progress);
                        }
                        reply->deleteLater();
                        return;
                }
				const std::vector<std::string> allowed_download_hosts{"github.com", "objects.githubusercontent.com", "gdownload.uk"};
				if (!ValidateDownloadUrl(reply->url().toString().toStdString(), allowed_download_hosts)) {
					last_error_ = "Update download redirect target is not allowed";
					update_in_progress_ = false;
					download_file_.close();
					QFile::remove(update_package_path_);
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
			if (installation_thread_.joinable()) installation_thread_.request_stop();
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

			if (!platform_verifier_ || !installer_launcher_ || !rollback_store_) {
				last_error_ = "Update trust services are unavailable";
				update_in_progress_ = false;
				return false;
			}
			if (installation_thread_.joinable()) return false;
			const auto package = update_package_path_.toStdString();
			const auto info = update_info_;
			installation_thread_ = std::jthread([this, package, info](std::stop_token stop_token) {
#ifdef _WIN32
				LockedUpdatePackage package_lock(package);
				if (!package_lock.IsValid()) {
					std::lock_guard lock(error_mutex_);
					last_error_ = "Failed to lock update package";
					update_in_progress_ = false;
					return;
				}
#endif
				InstallationGate gate(*platform_verifier_, *installer_launcher_, *rollback_store_);
				const auto result = gate.Install(package, info.package_size, info.sha256,
					GDOWNLOAD_UPDATE_SIGNER_SPKI_PIN, info.release_id, stop_token);
				update_in_progress_ = false;
				if (!result.Succeeded()) {
					std::lock_guard lock(error_mutex_);
					last_error_ = result.detail.empty() ? "Update installation failed" : result.detail;
				}
				const auto callback = progress_callback_;
				const auto alive = alive_;
				const UpdateProgress progress{result.Succeeded() ? UpdateProgress::Stage::kFinished :
					UpdateProgress::Stage::kFailed, 100, result.Succeeded() ? "Update installed successfully" : result.detail};
				if (callback && QCoreApplication::instance()) {
					QMetaObject::invokeMethod(QCoreApplication::instance(), [alive, callback, progress] {
						if (alive->load()) callback(progress);
					}, Qt::QueuedConnection);
				}
			});
			return true;

            // 执行安装包前再次校验 SHA-256（TOCTOU 缓解）（S2）
            if (!update_info_.sha256.empty()) {
                const QString actual_sha = ComputeFileSha256(update_package_path_);
                if (actual_sha.isEmpty() ||
                    actual_sha.compare(QString::fromStdString(update_info_.sha256), Qt::CaseInsensitive) != 0) {
                    last_error_ = "Update package SHA-256 mismatch before install";
                    update_in_progress_ = false;
                    if (progress_callback_) {
                        UpdateProgress progress;
                        progress.stage = UpdateProgress::Stage::kFailed;
                        progress.percentage = 100;
                        progress.message = "SHA-256 verification failed";
                        progress_callback_(progress);
                    }
                    return false;
                }
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

				const auto rollback_state = rollback_store_ ? rollback_store_->HighestReleaseId() :
					RollbackReadResult{false, 0, "rollback store unavailable"};
				if (!rollback_state.ok) {
					last_error_ = rollback_state.error;
					if (callback) callback(false, UpdateInfo{});
					return;
				}
                const ManifestPolicy policy{"windows-x64", ".exe",
                    {"github.com", "objects.githubusercontent.com", "gdownload.uk"},
					rollback_state.value,
                    QDateTime::currentSecsSinceEpoch()};
                const auto verified = VerifyUpdateManifest(data.toStdString(),
                    GDOWNLOAD_UPDATE_PUBLIC_KEY_BASE64, policy);
                if (!verified.ok) {
                    last_error_ = verified.error;
                    if (!is_fallback && !config_.fallback_update_url.empty()) {
                        startCheckRequest(config_.fallback_update_url, true, callback);
                        return;
                    }
                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }
                const auto& manifest = verified.manifest;
                VersionTools::Version new_version(manifest.version);
                VersionTools::Version old_version(config_.current_version);
                if (new_version <= old_version) {
                    // 本轮检查成功完成(已是最新版):清掉主源失败、fallback 成功
                    // 场景下残留的错误,避免上层把成功的检查误判为失败
                    last_error_.clear();
                    if (callback) {
                        callback(false, UpdateInfo{});
                    }
                    return;
                }
                UpdateInfo info;
                info.release_id = manifest.release_id;
                info.version = manifest.version;
                info.asset_name = manifest.asset.name;
                info.published_at = manifest.published_at;
                info.expires_at = manifest.expires_at;
                info.release_date = QDateTime::fromSecsSinceEpoch(manifest.published_at).toString("yyyy-MM-dd hh:mm:ss").toStdString();
                info.release_notes = NormalizeReleaseNotes(manifest.notes);
                info.download_url = manifest.asset.url;
                info.package_size = manifest.asset.size;
                info.sha256 = manifest.asset.sha256;
                last_error_.clear();
                update_available_ = true;
                update_info_ = info;
                if (callback) callback(true, info);
                return;

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
