#pragma once
#include <QtQml/qqml.h>
#include <QObject>
#include <QVariantMap>
#include "Aria2CManager/aria2c_manager.h"
#include "download_task_model.h"
#include "singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {
    namespace ui {
        namespace browser {

            Q_NAMESPACE
            class BrowserManager : public QObject, public Singleton<BrowserManager> {
                Q_OBJECT
                SINGLETON_DECLARE(BrowserManager)
                QML_SINGLETON
               public:
                static BrowserManager* create(QQmlEngine*, QJSEngine*);

               public:
                ~BrowserManager() override;
                Q_INVOKABLE DownloadTaskModel* GetActiveDownloadModel();

                Q_INVOKABLE DownloadTaskModel* GetStopedDownloadModel();

                Q_INVOKABLE DownloadTaskModel* GetWaitingDownloadModel();

                Q_INVOKABLE bool AddHttpTask(const QVariantList& urls, const QVariantMap& options);

                Q_INVOKABLE bool AddTorrentTask(const QString& tarrent, const QVariantMap& options);

                Q_INVOKABLE bool AddMetalinkTask(const QString& metalink, const QVariantMap& options);

                Q_INVOKABLE bool PauseTask(const QString& gid);

                Q_INVOKABLE bool PauseAllTask();

                Q_INVOKABLE bool ForcePauseTask(const QString& gid);

                Q_INVOKABLE bool ForcePauseAllTask();

                Q_INVOKABLE bool UnpauseTask(const QString& gid);

                Q_INVOKABLE bool UnpauseAllTask();

                Q_INVOKABLE bool RemoveTask(const QString& gid);

                Q_INVOKABLE bool ForceRemoveTask(const QString& gid);

                Q_INVOKABLE bool RemoveDownloadResult(const QString& gid);

                Q_INVOKABLE bool PurgeDownloadResult();

                Q_INVOKABLE bool ChangeOption(const QString& gid, const QVariantMap& options);

                Q_INVOKABLE bool ChangeGlobalOption(const QVariantMap& options);

                bool Init();
                void UnInit();

               public:
               Q_SIGNALS:
                void sigErrorMessage(const QString& error);
                void sigUpdateTasksMessage(const QString& data);

               private:
                explicit BrowserManager(QObject* parent = nullptr);
                void OnHandleAria2Message(const std::string& msg);

               private:
                std::unique_ptr<DownloadTaskModel> active_model_{nullptr};
                std::unique_ptr<DownloadTaskModel> stoped_model_{nullptr};
                std::unique_ptr<DownloadTaskModel> waiting_model_{nullptr};
                engine::Subscription subcription_{nullptr};
            };
            void RegisterTypes(QQmlEngine* engine);
        }  // namespace browser
    }  // namespace ui
}  // namespace gdl
