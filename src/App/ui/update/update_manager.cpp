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
			// 自动检查开启且间隔有效时:
			// 1. 每次启动都在 5 秒后静默检查一次(不再受"距上次检查 >= 间隔"限制,
			//    发现新版才弹窗,无更新时静默无打扰);
			// 2. 周期定时器保留,应用长期不重启也能发现新版。
			if (config.check_interval_hours > 0 && config.enable_auto_check) {
				check_timer_.setInterval(config.check_interval_hours * 60 * 60 * 1000);
				check_timer_.start();
				QTimer::singleShot(5000, this, [this]() { CheckForUpdates(true); });
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
			// 清空上一轮残留错误:"成功但无更新"不会写 last_error_,不清空会把
			// 陈旧错误误判为本轮检查失败(见 checkForUpdatesFinished 的 error 判定)
			last_error_.clear();
			updater_->ClearLastError();
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
			// 手动检查(非静默)无论结果都发回执:设置页按钮据此复位忙碌态并提示
			// "已是最新版本"/"检查失败: 原因";静默启动检查不发射,避免误弹提示。
			// error 非空即本轮检查失败(CheckForUpdates 入口已清空陈旧错误)
			if (!silent_check_) {
				const QString error =
					updater_ ? QString::fromStdString(updater_->GetLastError()) : QString();
				QMetaObject::invokeMethod(
					this, [this, has_update, error]() { Q_EMIT checkForUpdatesFinished(has_update, error); },
					Qt::QueuedConnection);
			}
			if (has_update) {
				latest_update_info_ = info;
				// 使用 QMetaObject::invokeMethod 确保在主线程中创建对象
				QMetaObject::invokeMethod(
					this,
					[this, info]() {
						// 复用单个信息对象，避免重复检查累积 QObject（M1）
						if (!update_data_) {
							update_data_ = new UpdateDataInfo(this);
						}
						update_data_->Setversion(QString::fromStdString(info.version));
						update_data_->Seturl(QString::fromStdString(info.download_url));
						update_data_->Setrelease_note(QString::fromStdString(info.release_notes));
						update_data_->Setrelease_date(QString::fromStdString(info.release_date));

						// If silent check and not mandatory update, only emit signal
						if (silent_check_ && !info.is_mandatory && !config_.silent_mode) {
							Q_EMIT updateAvailable(update_data_);
							return;
						}
						// If mandatory update or silent mode enabled, start update automatically
						if (info.is_mandatory || config_.silent_mode) {
							StartUpdate();
						}
						else {
							// Otherwise just emit signal
							Q_EMIT updateAvailable(update_data_);
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
				// 复用单个进度对象，避免每次进度回调 new 一个以单例为父的 QObject 泄漏（M1）
				if (!progress_data_) {
					progress_data_ = new UpdateProgressData(this);
				}
				progress_data_->Setstage(stage);
				progress_data_->Setpercentage(percentage);
				progress_data_->Setmessage(message);
				Q_EMIT updateProgress(progress_data_);
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
