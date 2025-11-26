#include "update_manager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QQmlEngine>
#include <QSettings>
#include "Definitions/appDef.h"
namespace gdl {
	namespace update {

		UpdateManager::UpdateManager(QObject* parent) : QObject(parent) {
			// Create platform-specific updater
			updater_ = AutoUpdater::Create();

			// Connect timer signal
			connect(&check_timer_, &QTimer::timeout, this, &UpdateManager::onAutoCheckTimer);
		}

		UpdateManager::~UpdateManager() {
			check_timer_.stop();
		}

		bool UpdateManager::Initialize(const UpdateConfig& config) {
			if (!updater_) {
				last_error_ = "Failed to create updater instance";
				return false;
			}
			config_ = config;
			// Initialize updater
			if (!updater_->Initialize(config)) {
				last_error_ = QString::fromStdString(updater_->GetLastError());
				return false;
			}
			// Read last check time
			QSettings settings;
			QDateTime last_check = settings.value("update/last_check_time").toDateTime();
			QDateTime now		 = QDateTime::currentDateTime();
			// If auto-check is enabled and interval is greater than 0
			if (config.check_interval_hours > 0 && config.disable_auto_check) {
				// Set timer to check at specified interval
				check_timer_.setInterval(config.check_interval_hours * 60 * 60 * 1000);
				check_timer_.start();

				// If never checked before or interval has passed, check immediately
				if (!last_check.isValid() || last_check.addSecs(config.check_interval_hours * 3600) <= now) {
					// Use silent mode for automatic check
					QTimer::singleShot(5000, [this]() { CheckForUpdates(true); });
				}
				else {
					QTimer::singleShot(5000, [this]() { CheckForUpdates(true); });
				}
			}
			return true;
		}

		void UpdateManager::CheckForUpdates(bool silent) {
			if (!updater_) {
				return;
			}

			silent_check_ = silent;
			// 记录检查时间
			QSettings settings;
			settings.setValue("update/last_check_time", QDateTime::currentDateTime());
			// 检查更新
			updater_->CheckForUpdates(
				[this](bool has_update, const UpdateInfo& info) { onUpdateCheckCompleted(has_update, info); });
		}

		bool UpdateManager::StartUpdate() {
			if (!updater_ || !update_available_) {
				last_error_ = "No update available";
				return false;
			}
			// Start update
			return updater_->StartUpdate([this](const UpdateProgress& progress) { onUpdateProgress(progress); });
		}

		void UpdateManager::CancelUpdate() {
			if (updater_) {
				updater_->CancelUpdate();
			}
		}

		QString UpdateManager::GetLastError() const {
			return last_error_;
		}

		void UpdateManager::SetRequestHeaders(const std::map<std::string, std::string>& headers) {
			if (updater_) {
				updater_->SetRequestHeaders(headers);
			}
		}

		void UpdateManager::onAutoCheckTimer() {
			// 自动检查使用静默模式
			CheckForUpdates(true);
		}

		void UpdateManager::onUpdateCheckCompleted(bool has_update, const UpdateInfo& info) {
			update_available_ = has_update;
			if (has_update) {
				latest_update_info_ = info;
				// 使用 QMetaObject::invokeMethod 确保在主线程中创建对象
				QMetaObject::invokeMethod(
					this,
					[this, info]() {
						auto data = new UpdateDataInfo(QString::fromStdString(info.version),
													   QString::fromStdString(info.download_url),
													   QString::fromStdString(info.release_notes),
													   QString::fromStdString(info.release_date), this);

						// If silent check and not mandatory update, only emit signal
						if (silent_check_ && !info.is_mandatory && !config_.silent_mode) {
							Q_EMIT updateAvailable(data);
							return;
						}
						// If mandatory update or silent mode enabled, start update automatically
						if (info.is_mandatory || config_.silent_mode) {
							StartUpdate();
						}
						else {
							// Otherwise just emit signal
							Q_EMIT updateAvailable(data);
						}
					},
					Qt::QueuedConnection);
			}
		}

		void UpdateManager::onUpdateProgress(const UpdateProgress& progress) {
			// 转发进度信号
			QString message = QString::fromStdString(progress.message);
			auto stage		= static_cast<int>(progress.stage);
			auto percentage = progress.percentage;
			// 使用 QMetaObject::invokeMethod 确保在主线程中创建对象
			QMetaObject::invokeMethod(this, [this, stage, percentage, message]() {
				auto data = new UpdateProgressData(stage, percentage, message, this);
				Q_EMIT updateProgress(data);
			}, Qt::QueuedConnection);
			
			// 检查更新是否完成
        if (progress.stage == UpdateProgress::Stage::kFinished) {
            apply_requested_ = false;
            QMetaObject::invokeMethod(this, [this]() {
                Q_EMIT updateFinished(true);
            }, Qt::QueuedConnection);
        }
        else if (progress.stage == UpdateProgress::Stage::kFailed) {
            apply_requested_ = false;
            QMetaObject::invokeMethod(this, [this, message]() {
                last_error_ = message;
                Q_EMIT updateFinished(false);
            }, Qt::QueuedConnection);
        }
        else if (progress.stage == UpdateProgress::Stage::kInstalling) {
            if (apply_requested_ || progress.percentage < 100) {
                return;
            }
            apply_requested_ = true;
            QMetaObject::invokeMethod(this, [this]() {
                updater_->ApplyUpdate();
            }, Qt::QueuedConnection);
        }
		}

		void RegisterTypes(QQmlEngine* engine) {
			qmlRegisterType<UpdateDataInfo>(GEXPORT_MODULE_URL, 1, 0, "UpdateDataInfo");
			qmlRegisterType<UpdateProgressData>(GEXPORT_MODULE_URL, 1, 0, "UpdateProgressData");
			qmlRegisterSingletonInstance<UpdateManager>(GEXPORT_MODULE_URL, 1, 0, "UpdateManager",
														&UpdateManager::Instance());
		}

	}  // namespace update
}  // namespace gdl
