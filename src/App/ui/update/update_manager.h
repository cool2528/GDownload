#pragma once
#include <QObject>
#include <QTimer>
#include <memory>
#include "Definitions/autoProperty.h"
#include "auto_updater.h"
#include "singleton.hpp"
class QQmlEngine;
namespace gdl {
    namespace update {
        class UpdateDataInfo : public QObject {
            Q_OBJECT
            QML_AUTO_PROPERTY(QString, version)
            QML_AUTO_PROPERTY(QString, url)
            QML_AUTO_PROPERTY(QString, release_note)
            QML_AUTO_PROPERTY(QString, release_date)
           public:
            explicit UpdateDataInfo(QObject* parent = nullptr) : QObject(parent) {}
            explicit UpdateDataInfo(const QString& version, const QString& url, const QString& release_note,
                                    const QString& release_date, QObject* parent = nullptr)
                : QObject(parent),
                  version_(version),
                  url_(url),
                  release_note_(release_note),
                  release_date_(release_date) {}
        };
        class UpdateProgressData : public QObject {
            Q_OBJECT
            QML_AUTO_PROPERTY(int, stage)
            QML_AUTO_PROPERTY(int, percentage)
            QML_AUTO_PROPERTY(QString, message)
           public:
            explicit UpdateProgressData(QObject* parent = nullptr) : QObject(parent) {}
            explicit UpdateProgressData(int stage, int percentage, const QString& message, QObject* parent = nullptr)
                : QObject(parent), stage_(stage), percentage_(percentage), message_(message) {}
        };

        class UpdateManager : public QObject, public Singleton<UpdateManager> {
            Q_OBJECT
            SINGLETON_DECLARE(UpdateManager)
           public:
            ~UpdateManager() override;

            // 初始化更新管理器
            bool Initialize(const UpdateConfig& config);

            // 手动检查更新
		  Q_INVOKABLE void CheckForUpdates(bool silent = false);

            // 开始安装更新
           Q_INVOKABLE bool StartUpdate();

            // 取消更新
		   Q_INVOKABLE void CancelUpdate();

            // 获取最后一次错误
		   Q_INVOKABLE QString GetLastError() const;

            // 设置自定义HTTP请求头
            void SetRequestHeaders(const std::map<std::string, std::string>& headers);

           Q_SIGNALS:
            // 发现更新信号
            void updateAvailable(UpdateDataInfo* info);

            // 更新进度信号
            void updateProgress(UpdateProgressData* progress);

            // 更新完成信号
            void updateFinished(bool success);

           private Q_SLOTS:
            void onAutoCheckTimer();
            void onUpdateCheckCompleted(bool has_update, const UpdateInfo& info);
            void onUpdateProgress(const UpdateProgress& progress);

           private:
            explicit UpdateManager(QObject* parent = nullptr);

           private:
            std::unique_ptr<AutoUpdater> updater_;
            QTimer check_timer_;
            UpdateConfig config_;
            UpdateInfo latest_update_info_;
            bool update_available_ = false;
            bool silent_check_	   = false;
            QString last_error_;
            bool apply_requested_ = false;
            UpdateProgressData* progress_data_ = nullptr;  // 复用的进度对象（M1）
            UpdateDataInfo* update_data_ = nullptr;        // 复用的更新信息对象（M1）
        };

        void RegisterTypes(QQmlEngine* engine);

    }  // namespace update
}  // namespace gdl
