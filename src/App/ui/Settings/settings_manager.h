#pragma once
#include <QtQml/qqml.h>
#include <QObject>
#include <QReadWriteLock>
#include "ISettings.h"
#include "setting.h"
#include "singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {
    namespace ui {
        namespace settings {

            // 下载设置实现类,继承 ISettings 纯虚接口与 Singleton<SettingsImpl>
            // QObject 必须置于基类列表首位,moc 依此链接 staticMetaObject;ISettings 为非 Q_OBJECT 纯虚接口
            // 保留 Q_INVOKABLE 以确保 moc 注册元对象信息,QML 侧方法调用不受影响
            class SettingsImpl : public QObject, public ISettings, public Singleton<SettingsImpl> {
                SINGLETON_DECLARE(SettingsImpl)
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
                // 追加在属性列表末尾:新增 Q_PROPERTY/Q_INVOKABLE 不移动已有 moc 索引,
                // 避免增量构建下 QML 元调用错位(读取属性时错调到其它 getter 导致类型混淆崩溃)
                SETTING_PROPERTY(QString, PluginSourceProxy)
                // eD2k 设置组(追加在属性列表末尾,同上避免 moc 索引错位)
                SETTING_PROPERTY(QString, Ed2kNickname)
                SETTING_PROPERTY(int, Ed2kTcpPort)
                SETTING_PROPERTY(int, Ed2kUdpPort)
                SETTING_PROPERTY(bool, Ed2kEnableKad)
                SETTING_PROPERTY(bool, Ed2kEnableObfuscation)
                SETTING_PROPERTY(bool, Ed2kAutoConnect)
                SETTING_PROPERTY(int, Ed2kMaxConcurrentTasks)
                SETTING_PROPERTY(QString, Ed2kSharedDirs)
                SETTING_PROPERTY(QString, Ed2kServerMetUrl)
                SETTING_PROPERTY(QString, Ed2kNodesDatUrl)

               public:
                ~SettingsImpl() override;
                static SettingsImpl* create(QQmlEngine*, QJSEngine*);
                bool Init();
                void UnInit();
                Q_INVOKABLE QString GenerateRpcSecret() const;
                Q_INVOKABLE QString GetDefaultBrowserUserAgent() const;
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

               protected:
                // 构造函数为 protected:允许测试替身 FakeSettingsManager 继承并构造基类子对象
                // Singleton<SettingsImpl> 作为 friend 仍可访问(friend 绕过访问控制)
                // 外部代码仍无法直接构造(protected 对非派生类不可见),单例约束保持不变
                explicit SettingsImpl(QObject* parent = nullptr);
               private:
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
            // 类型别名:保现有 Settings::Instance() 调用点零改动
            using Settings = SettingsImpl;
            void RegisterTypes(QQmlEngine* engine);
        }  // namespace settings
    }  // namespace ui
}  // namespace gdl
