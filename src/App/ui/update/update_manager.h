#pragma once
#include <QObject>
#include <QTimer>
#include <memory>
#include "auto_updater.h"
#include "singleton.hpp"
class QQmlEngine;
namespace gdl {
    namespace update {

        class UpdateManager : public QObject, public Singleton<UpdateManager> {
            Q_OBJECT
            SINGLETON_DECLARE(UpdateManager)
           public:
            ~UpdateManager() override;

            // 初始化更新管理器
            bool Initialize(const UpdateConfig& config);

            // 手动检查更新
            void CheckForUpdates(bool silent = false);

            // 开始安装更新
            bool StartUpdate();

            // 取消更新
            void CancelUpdate();

            // 获取最后一次错误
            QString GetLastError() const;

            // 设置自定义HTTP请求头
            void SetRequestHeaders(const std::map<std::string, std::string>& headers);

           Q_SIGNALS:
            // 发现更新信号
            void updateAvailable(const UpdateInfo& info);

            // 更新进度信号
            void updateProgress(const UpdateProgress& progress);

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
        };

        void RegisterTypes(QQmlEngine* engine);

    }  // namespace update
}  // namespace gdl
