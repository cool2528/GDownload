#pragma once
#include <QtQml/qqml.h>
#include <QObject>
#include <QReadWriteLock>
#include "setting.h"
#include "singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {
    namespace ui {
        namespace settings {

            class Settings : public QObject, public Singleton<Settings> {
                SINGLETON_DECLARE(Settings)
                Q_OBJECT
                QML_SINGLETON
                SETTING_PROPERTY(QSize, WindowSize)
                SETTING_PROPERTY(QString, Theme)
                SETTING_PROPERTY(QString, Language)
                SETTING_PROPERTY(QString, BtExludeTracker)
                SETTING_PROPERTY(QString, BtTracker)
                SETTING_PROPERTY(QString, Dir)
                SETTING_PROPERTY(int, ListenPort)
                SETTING_PROPERTY(int, RpcListenPort)
                SETTING_PROPERTY(QString, RpcSecret)
                SETTING_PROPERTY(int, Split)
                SETTING_PROPERTY(QString, UserAgent)
                SETTING_PROPERTY(QString, AllProxy)
                SETTING_PROPERTY(int, DhtListenPort)
                SETTING_PROPERTY(int, MaxConcurrentDownloads)
                SETTING_PROPERTY(QString, ConfPath)
				SETTING_PROPERTY(QString, TrackerSourceUrls)
                SETTING_PROPERTY(QString, SaveSession)
                SETTING_PROPERTY(bool, IsSaveSession)
                SETTING_PROPERTY(bool, EnableGlobalProxy)
                SETTING_PROPERTY(QString, GlobalProxy)
                SETTING_PROPERTY(bool, ListenClipboard)
                SETTING_PROPERTY(bool, AutoResumeTask)
                SETTING_PROPERTY(bool, AutoStart)
                SETTING_PROPERTY(bool, RememberWindowPosition)
                SETTING_PROPERTY(bool, EnableTrayIcon)
                SETTING_PROPERTY(bool, EnableNotification)
                SETTING_PROPERTY(bool, EnableAutoShutdown)
                SETTING_PROPERTY(bool, EnableAutoUpdate)
                SETTING_PROPERTY(bool, EnableGithubAccelerate)
                SETTING_PROPERTY(QPoint, WindowPosition)
                SETTING_PROPERTY(QString, BaiduPanCookies)
                SETTING_PROPERTY(QString, TrackerSourceNames)
                SETTING_PROPERTY(bool, EnableTrackerSourceAutoUpdate)
                SETTING_PROPERTY(bool, ShowCloseConfirm)
                SETTING_PROPERTY(bool, CloseToTray)
                SETTING_PROPERTY(int, MaxDownloadLimit)
                SETTING_PROPERTY(int, MaxOverallDownloadLimit)
                SETTING_PROPERTY(int, MaxUploadLimit)
                SETTING_PROPERTY(int, MaxOverallUploadLimit)
                SETTING_PROPERTY(int, LowestSpeedLimit)
                SETTING_PROPERTY(int, MaxConnectionPerServer)
                SETTING_PROPERTY(int, MinSplitSize)
                SETTING_PROPERTY(int, OnCompleteAction)
                SETTING_PROPERTY(QString, CustomCompleteCommand)
                SETTING_PROPERTY(int, OnErrorAction)
                SETTING_PROPERTY(QString, CustomErrorCommand)
                SETTING_PROPERTY(int, OnStartAction)
                SETTING_PROPERTY(int, Timeout)
                SETTING_PROPERTY(int, ConnectTimeout)
                SETTING_PROPERTY(int, MaxTries)
                SETTING_PROPERTY(int, RetryWait)
                SETTING_PROPERTY(bool, EnableDht)
                SETTING_PROPERTY(int, BtMaxPeers)
                SETTING_PROPERTY(bool, BtRequireCrypto)

               public:
                ~Settings() override;
                static Settings* create(QQmlEngine*, QJSEngine*);
                bool Init();
                void UnInit();
                Q_INVOKABLE QString GenerateRpcSecret() const;
                Q_INVOKABLE void SetAria2Dir(const QString& dir);
                Q_INVOKABLE void SetAria2GlobalProxy(const QString& proxy);
                Q_INVOKABLE void SetAria2AutoResumeTask(bool enable);
                Q_INVOKABLE void SetAria2MaxDownloadLimit(int value);
                Q_INVOKABLE void SetAria2MaxOverallDownloadLimit(int value);
                Q_INVOKABLE void SetAria2MaxUploadLimit(int value);
                Q_INVOKABLE void SetAria2MaxOverallUploadLimit(int value);
                Q_INVOKABLE void SetAria2LowestSpeedLimit(int value);
                Q_INVOKABLE void SetAria2MaxConcurrentDownloads(int value);
                Q_INVOKABLE void SetAria2Split(int value);
                Q_INVOKABLE void SetAria2MaxConnectionPerServer(int value);
                Q_INVOKABLE void SetAria2MinSplitSize(int sizeMB);
                Q_INVOKABLE void SetAria2Timeout(int value);
                Q_INVOKABLE void SetAria2ConnectTimeout(int value);
                Q_INVOKABLE void SetAria2MaxTries(int value);
                Q_INVOKABLE void SetAria2RetryWait(int value);
                Q_INVOKABLE void SetAria2EnableDht(bool enable);
                Q_INVOKABLE void SetAria2BtMaxPeers(int value);
                Q_INVOKABLE void SetAria2BtRequireCrypto(bool enable);
                Q_INVOKABLE void SetAria2UserAgent(const QString& userAgent);

               private:
                explicit Settings(QObject* parent = nullptr);
                void Save();
                template <class SETTING, class VALUE_TYPE>
                void SetValue(const QString& key, const VALUE_TYPE& value) {
                    auto match = settings::Setting::settings_.find(key);
                    if (match == settings::Setting::settings_.end()) {
                        return;
                    }
                    if (auto setting_ptr = dynamic_cast<SETTING*>(match.value()); setting_ptr) {
                        lock_.lockForWrite();
                        setting_ptr->Put(value);
                        Save();
                        lock_.unlock();
                    }
                }

                template <class SETTING, class VALUE_TYPE>
                VALUE_TYPE GetValue(const QString& key) const {
                    auto match = settings::Setting::settings_.find(key);
                    if (match == settings::Setting::settings_.end()) {
                        return VALUE_TYPE();
                    }
                    if (auto setting_ptr = dynamic_cast<SETTING*>(match.value()); setting_ptr) {
                        lock_.lockForRead();
                        VALUE_TYPE res = setting_ptr->Get();
                        lock_.unlock();
                        return res;
                    }
                    return VALUE_TYPE();
                }

               private:
                mutable QReadWriteLock lock_;
            };
            void RegisterTypes(QQmlEngine* engine);
        }  // namespace settings
    }  // namespace ui
}  // namespace gdl
