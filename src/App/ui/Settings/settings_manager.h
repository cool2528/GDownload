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
                SETTING_PROPERTY(QPoint, WindowPosition)

               public:
                ~Settings() override;
                static Settings* create(QQmlEngine*, QJSEngine*);
                bool Init();
                void UnInit();

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
