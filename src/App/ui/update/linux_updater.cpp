#include "linux_updater.h"
#if defined(__linux__)
#include <appimage/update.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <chrono>
#include <nlohmann/json.hpp>
#include <thread>
#include "update/linux_appimage_gate.h"
#include "update/update_manifest.h"
#include "update_trust_config.h"
#include "file_update_rollback_store.h"
#include "os/os.h"

namespace {
	std::filesystem::path RollbackStatePath() {
		return std::filesystem::path(gdl::os::GetAppDataDir()) / "gdownload" / "update" / "highest_release_id";
	}
	class AppImageUpdaterVerifier final : public gdl::update::ILinuxAppImageVerifier {
	 public:
		explicit AppImageUpdaterVerifier(appimage::update::Updater& updater) : updater_(updater) {}
		gdl::update::AppImageSignatureState Verify(const std::filesystem::path&) const override {
			const auto state = updater_.validateSignature();
			using Updater = appimage::update::Updater;
			if (state == Updater::VALIDATION_PASSED) return gdl::update::AppImageSignatureState::kPassed;
			if (state == Updater::VALIDATION_NOT_SIGNED) return gdl::update::AppImageSignatureState::kUnsigned;
			if (state == Updater::VALIDATION_KEY_CHANGED || state == Updater::VALIDATION_NO_LONGER_SIGNED)
				return gdl::update::AppImageSignatureState::kKeyChanged;
			return gdl::update::AppImageSignatureState::kInvalid;
		}
	 private:
		appimage::update::Updater& updater_;
	};

	class AppImageLauncher final : public gdl::update::ILinuxAppImageLauncher {
	 public:
		explicit AppImageLauncher(bool restart) : restart_(restart) {}
		gdl::update::InstallationResult Launch(const std::filesystem::path& package) override {
			QFile file(QString::fromStdString(package.string()));
			if (!file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser |
				QFile::ExeGroup | QFile::ExeOther))
				return {gdl::update::InstallationStatus::kLaunchFailed, 0, "failed to set AppImage permissions"};
			if (!restart_) return {gdl::update::InstallationStatus::kSucceeded, 0, {}};
			if (!QProcess::startDetached(QString::fromStdString(package.string()), QStringList()))
				return {gdl::update::InstallationStatus::kLaunchFailed, 0, "failed to start updated AppImage"};
			QCoreApplication::quit();
			return {gdl::update::InstallationStatus::kSucceeded, 0, {}};
		}
	 private:
		bool restart_{false};
	};

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
}  // namespace

namespace gdl {
	namespace update {

		LinuxUpdater::LinuxUpdater() = default;

		LinuxUpdater::~LinuxUpdater() {
			alive_->store(false);
			CancelUpdate();
		}

		bool LinuxUpdater::Initialize(const UpdateConfig& config) {
			config_ = config;

			// Ensure temporary directory exists
			if (config_.temp_dir.empty()) {
				config_.temp_dir =
					QStandardPaths::writableLocation(QStandardPaths::TempLocation).toStdString() + "/GDownloadUpdater";
			}

			QDir dir(QString::fromStdString(config_.temp_dir));
			if (!dir.exists() && !dir.mkpath(".")) {
				last_error_ = "Failed to create temporary directory: " + config_.temp_dir;
				qCritical() << "Failed to create temporary directory:" << QString::fromStdString(config_.temp_dir);
				return false;
			}
			QString appimage_path = QCoreApplication::applicationFilePath();

			// Check if we're running in an AppImage
			if (qEnvironmentVariableIsSet("APPIMAGE")) {
				appimage_path = qEnvironmentVariable("APPIMAGE");
			}

			if (!updater_) {
				updater_ = std::make_unique<appimage::update::Updater>(appimage_path.toStdString());
			}

			return true;
		}

		void LinuxUpdater::handleNetworkReply(QNetworkReply* reply, bool is_fallback, UpdateCheckCallback callback) {
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

			QString appimage_path = QCoreApplication::applicationFilePath();

			// Check if we're running in an AppImage
			if (qEnvironmentVariableIsSet("APPIMAGE")) {
				appimage_path = qEnvironmentVariable("APPIMAGE");
			}

			QByteArray data = reply->readAll();
			try {

				FileUpdateRollbackStore rollback_store(RollbackStatePath());
				const auto rollback = rollback_store.HighestReleaseId();
				if (!rollback.ok) {
					last_error_ = rollback.error;
					if (callback) callback(false, UpdateInfo{});
					return;
				}
				const ManifestPolicy policy{"linux-x86_64", ".AppImage",
					{"github.com", "objects.githubusercontent.com", "gdownload.uk"},
					rollback.value,
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
				UpdateInfo info;
				info.release_id = manifest.release_id; info.version = manifest.version;
				info.asset_name = manifest.asset.name; info.published_at = manifest.published_at;
				info.expires_at = manifest.expires_at; info.release_notes = NormalizeReleaseNotes(manifest.notes);
				info.release_date = QDateTime::fromSecsSinceEpoch(manifest.published_at)
					.toString("yyyy-MM-dd hh:mm:ss").toStdString();
				info.download_url = manifest.asset.url; info.package_size = manifest.asset.size;
				info.sha256 = manifest.asset.sha256; update_info_ = info;
				bool update_available = false;
				if (!updater_->checkForChanges(update_available)) {
					// AppImage updater 本地检查失败（如读取 AppImage 更新信息、校验等），
					// 属于本地错误而非更新源（GitHub API / fallback URL）不可用，
					// 因此不应 fallback 到 GitHub API，直接回调失败即可。
					std::string message;
					while (updater_->nextStatusMessage(message)) {
						last_error_ += message + "\n";
						qWarning() << "Update check error:" << QString::fromStdString(message);
					}
					if (callback) {
						callback(false, UpdateInfo{});
					}
					return;
				}
				// 更新检查成功完成，清空可能残留的上次检查错误信息
				last_error_.clear();
				update_available_ = update_available;
				if (update_available) {
					std::string message;
					while (updater_->nextStatusMessage(message)) {
						qInfo() << "Update info:" << QString::fromStdString(message);
					}
				}
				if (callback) {
					callback(update_available, update_info_);
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
		void LinuxUpdater::CheckForUpdates(UpdateCheckCallback callback) {
			startCheckRequest(config_.update_url, false, callback);
		}
		void LinuxUpdater::startCheckRequest(const std::string& update_url, bool is_fallback,
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
			current_reply_ = reply;  // 保存以便 CancelUpdate 能中止请求
			QObject::connect(reply, &QNetworkReply::finished, [this, reply, is_fallback, callback, alive = alive_]() {
				if (!alive->load()) {
					reply->deleteLater();
					return;
				}
				this->handleNetworkReply(reply, is_fallback, callback);
				if (current_reply_ == reply) {
					current_reply_ = nullptr;
				}
				reply->deleteLater();
			});
		}

		bool LinuxUpdater::StartUpdate(ProgressCallback progress_callback) {
			if (!update_available_ || update_in_progress_) {
				last_error_ = "No available update or update already in progress";
				qWarning() << "StartUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}

			progress_callback_	= progress_callback;
			update_in_progress_ = true;
			should_stop_thread_ = false;

			// Get current AppImage path
			QString appimage_path = QCoreApplication::applicationFilePath();

			// Check if we're running in an AppImage
			if (qEnvironmentVariableIsSet("APPIMAGE")) {
				appimage_path = qEnvironmentVariable("APPIMAGE");
			}

			// Check if it's an AppImage
			if (!appimage_path.contains(".AppImage")) {
				last_error_			= "Current application is not in AppImage format, cannot use AppImageUpdate";
				update_in_progress_ = false;
				qCritical() << "StartUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}

			try {
				// If updater_ is null, create a new instance
				if (!updater_) {
					updater_ = std::make_unique<appimage::update::Updater>(appimage_path.toStdString());
				}

				// Start update thread
				update_thread_ = std::thread([this, alive = alive_]() {
					try {
						if (!alive->load()) return;
						// Start update
						updater_->start();

						// Monitor update progress
						if (alive->load()) {
							UpdateProgressThread();
						}
					} catch (const std::exception& e) {
						if (!alive->load()) return;
						qCritical() << "Exception in update thread:" << e.what();
						last_error_			= std::string("Update thread error: ") + e.what();
						update_in_progress_ = false;

						if (progress_callback_) {
							UpdateProgress progress;
							progress.stage		= UpdateProgress::Stage::kFailed;
							progress.percentage = 0;
							progress.message	= "Update failed: " + std::string(e.what());
							progress_callback_(progress);
						}
					}
				});

				return true;
			} catch (const std::exception& e) {
				last_error_			= std::string("Failed to start update: ") + e.what();
				update_in_progress_ = false;
				qCritical() << "StartUpdate failed:" << e.what();
				return false;
			}
		}

		void LinuxUpdater::UpdateProgressThread() {
			// Monitor update progress until completion or cancellation
			while (!updater_->isDone() && !should_stop_thread_) {
				// Avoid high CPU usage
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				// Get progress
				double progress = 0.0;
				if (updater_->progress(progress)) {
					if (progress_callback_) {
						UpdateProgress update_progress;
						update_progress.percentage = static_cast<int>(progress * 100);
						// Get status message
						std::string message;
						if (updater_->nextStatusMessage(message)) {
							update_progress.message = message;
							qInfo() << "Update progress:" << QString::fromStdString(message);
						}
						else {
							update_progress.stage = UpdateProgress::Stage::kDownloading;
							update_progress.message =
								"Downloading update package: " + std::to_string(update_progress.percentage);
						}
						progress_callback_(update_progress);
					}
				}

				// Log all messages
				LogMessages();
			}

			// Update completed, check for errors
			if (!should_stop_thread_) {
				if (updater_->hasError()) {
					qCritical() << "Update failed with error";
					if (progress_callback_) {
						UpdateProgress progress;
						progress.stage		= UpdateProgress::Stage::kFailed;
						progress.percentage = 0;

						// Get error message
						std::string message;
						if (updater_->nextStatusMessage(message)) {
							progress.message = "Update failed: " + message;
							qCritical() << "Update error:" << QString::fromStdString(message);
						}
						else {
							progress.message = "Update failed";
							qCritical() << "Update failed with no specific error message";
						}

						progress_callback_(progress);
					}
				}
				else {
					qInfo() << "Update completed successfully";
					if (progress_callback_) {
						UpdateProgress progress;
						progress.stage		= UpdateProgress::Stage::kVerifying;
						progress.percentage = 100;
						progress.message	= "Update completed, preparing to apply update";
						progress_callback_(progress);
					}
				}
			}

			update_in_progress_ = false;
		}

		void LinuxUpdater::LogMessages() {
			// 仅记录日志，正常进度消息不应污染 last_error_（GetLastError 仅应返回真实错误）。
			std::string message;
			while (updater_->nextStatusMessage(message)) {
				if (!message.empty()) {
					qInfo() << "Update message:" << QString::fromStdString(message);
				}
			}
		}

		void LinuxUpdater::CancelUpdate() {
			if (current_reply_) {
				current_reply_->abort();
				current_reply_->deleteLater();
				current_reply_ = nullptr;
			}

			// Mark that the thread should stop
			should_stop_thread_ = true;

			// Wait for thread to finish
			if (update_thread_.joinable()) {
				update_thread_.join();
			}

			update_in_progress_ = false;
			qInfo() << "Update cancelled";
		}

		bool LinuxUpdater::ApplyUpdate(bool restart_app) {
			if (!updater_ || updater_->hasError()) {
				last_error_ = "No prepared update package or error occurred during update process";
				qCritical() << "ApplyUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}

			// Get new file path
			std::string new_file_path;
			if (!updater_->pathToNewFile(new_file_path) || new_file_path.empty()) {
				last_error_ = "Failed to get updated AppImage path";
				qCritical() << "ApplyUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}
			AppImageUpdaterVerifier verifier(*updater_);
			AppImageLauncher launcher(restart_app);
			FileUpdateRollbackStore rollback_store(RollbackStatePath());
			LinuxAppImageGate gate(verifier, launcher, rollback_store,
				GDOWNLOAD_REQUIRE_APPIMAGE_SIGNATURE != 0);
			const auto result = gate.Apply(new_file_path, update_info_.package_size,
				update_info_.sha256, update_info_.release_id);
			if (!result.Succeeded()) {
				last_error_ = result.detail.empty() ? "Updated AppImage trust handoff failed" : result.detail;
				if (progress_callback_) {
					progress_callback_({UpdateProgress::Stage::kFailed, 100, last_error_});
				}
				return false;
			}
			if (progress_callback_) {
				progress_callback_({UpdateProgress::Stage::kFinished, 100, "Update completed"});
			}
			return true;
		}

	}  // namespace update
}  // namespace gdl
#endif
