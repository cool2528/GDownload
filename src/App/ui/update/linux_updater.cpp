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

namespace {
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

		void LinuxUpdater::handleNetworkReply(QNetworkReply* reply, UpdateCheckCallback callback) {
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

			QString appimage_path = QCoreApplication::applicationFilePath();

			// Check if we're running in an AppImage
			if (qEnvironmentVariableIsSet("APPIMAGE")) {
				appimage_path = qEnvironmentVariable("APPIMAGE");
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
				QString tag_name	 = QString::fromStdString(doc["tag_name"].get<std::string>());
				tag_name			 = tag_name.replace("v", "");
				info.version		 = tag_name.toStdString();
				QString published_at = QString::fromStdString(doc["published_at"].get<std::string>());
				info.release_date =
					QDateTime::fromString(published_at, Qt::ISODate).toString("yyyy-MM-dd hh:mm:ss").toStdString();
				info.release_notes = NormalizeReleaseNotes(doc["body"].get<std::string>());
				if (doc.contains("assets") && doc["assets"].is_array()) {
					for (auto asset : doc["assets"]) {
						if (asset.contains("browser_download_url") && asset.contains("name")) {
							const auto name = asset["name"].get<std::string>();
							if (name.find(".AppImage") == std::string::npos) {
								continue;
							}
							info.download_url = asset["browser_download_url"].get<std::string>();
							info.package_size = asset["size"].get<int64_t>();
							break;
						}
					}
					update_info_ = info;
				}
				bool update_available = false;
				if (!updater_->checkForChanges(update_available)) {
					// Log error messages
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
				if (callback) {
					callback(false, UpdateInfo{});
				}
				return;
			}
		}
		void LinuxUpdater::CheckForUpdates(UpdateCheckCallback callback) {
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
				update_thread_ = std::thread([this]() {
					try {
						// Start update
						updater_->start();

						// Monitor update progress
						UpdateProgressThread();
					} catch (const std::exception& e) {
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
						progress.stage		= UpdateProgress::Stage::kInstalling;
						progress.percentage = 100;
						progress.message	= "Update completed, preparing to apply update";
						progress_callback_(progress);
					}
				}
			}

			update_in_progress_ = false;
		}

		void LinuxUpdater::LogMessages() {
			// Log all messages
			std::string message;
			while (updater_->nextStatusMessage(message)) {
				if (!message.empty()) {
					last_error_ += message + "\n";
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

			if (progress_callback_) {
				UpdateProgress progress;
				progress.stage		= UpdateProgress::Stage::kInstalling;
				progress.percentage = 100;
				progress.message	= "Preparing to apply update...";
				progress_callback_(progress);
			}

			// Ensure new file exists
			if (!QFile::exists(QString::fromStdString(new_file_path))) {
				last_error_ = "Updated AppImage file does not exist";
				qCritical() << "ApplyUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}

			// Set executable permissions
			QFile file(QString::fromStdString(new_file_path));
			if (!file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup |
									 QFile::ExeOther)) {
				last_error_ = "Failed to set executable permissions on updated AppImage";
				qCritical() << "ApplyUpdate failed:" << QString::fromStdString(last_error_);
				return false;
			}

			qInfo() << "Update applied successfully, new file path:" << QString::fromStdString(new_file_path);

			if (restart_app) {
				// Start new version
				qInfo() << "Starting new version and quitting current application";
				QProcess::startDetached(QString::fromStdString(new_file_path), QStringList());

				// Quit current application
				QCoreApplication::quit();
			}

			if (progress_callback_) {
				UpdateProgress progress;
				progress.stage		= UpdateProgress::Stage::kFinished;
				progress.percentage = 100;
				progress.message	= "Update completed";
				progress_callback_(progress);
			}

			return true;
		}

	}  // namespace update
}  // namespace gdl
#endif
