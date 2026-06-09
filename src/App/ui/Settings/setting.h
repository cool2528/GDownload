#pragma once
#include <qvariant.h>
#include <QHash>
#include <QObject>
#include <QRect>
#include <QSize>
#include <nlohmann/json.hpp>
#include "config/config_key.h"
namespace gdl {
	namespace ui {
		namespace settings {
			class Setting {
			   public:
				explicit Setting(const QString& key) : key_(key) { settings_.insert(key_, this); }
				virtual void Default()					= 0;
				virtual void Put(const QVariant& value) = 0;
				virtual QString ToString()				= 0;

			   protected:
				QString key_;

			   public:
				inline static QHash<QString, Setting*> settings_;
			};

#define CONFIG_KEY_PATH(NAME) config::Keys::NAME.get().data()

#define SETTING_IMP_BEGIN(CLASS_NAME, KEY, TYPE)      \
	class CLASS_NAME : public Setting {               \
	   public:                                        \
		CLASS_NAME() : Setting(KEY) {}                \
		using VALUE_TYPE					  = TYPE; \
		inline static const char* setting_key = KEY;  \
                                                      \
	   protected:                                     \
		VALUE_TYPE value_;                            \
                                                      \
	   public:

#define SETTING_IMP_END(CLASS_NAME) \
	}                               \
	;                               \
	inline static CLASS_NAME CLASS_NAME##Instance;

#define SETTING_PROPERTY(TYPE, NAME)                                             \
   private:                                                                      \
	Q_PROPERTY(TYPE q##NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) \
   public:                                                                       \
	Q_INVOKABLE TYPE Get##NAME() const {                                         \
		return GetValue<NAME, TYPE>(NAME::setting_key);                          \
	}                                                                            \
	Q_INVOKABLE void Set##NAME(TYPE value) {                                     \
		SetValue<NAME, TYPE>(NAME::setting_key, value);                          \
		Q_EMIT NAME##Changed();                                                  \
	}                                                                            \
	Q_SIGNAL void NAME##Changed();

			// WindowSize
			SETTING_IMP_BEGIN(WindowSize, CONFIG_KEY_PATH(WindowSize), QSize)
			void Default() override {
				value_ = QSize(1024, 768);
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<QSize>()) {
					value_ = value.toSize();
				}
				else if (value.canConvert<QString>()) {
					QString size = value.toString();
					int width	 = size.section(',', 0, 0).toInt();
					int height	 = size.section(',', 1, 1).toInt();
					value_		 = QSize(width, height);
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString("%1,%2").arg(value_.width()).arg(value_.height());
			}
			SETTING_IMP_END(WindowSize)

			// Theme
			SETTING_IMP_BEGIN(Theme, CONFIG_KEY_PATH(Theme), QString)
			void Default() override {
				value_ = "Light";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(Theme)

			// Language
			SETTING_IMP_BEGIN(Language, CONFIG_KEY_PATH(Language), QString)
			void Default() override {
				value_ = "zh-cn";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(Language)

			// BtExludeTracker
			SETTING_IMP_BEGIN(BtExludeTracker, CONFIG_KEY_PATH(BtExludeTracker), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(BtExludeTracker)

			// BtTracker
			SETTING_IMP_BEGIN(BtTracker, CONFIG_KEY_PATH(BtTracker), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(BtTracker)

			// Dir
			SETTING_IMP_BEGIN(Dir, CONFIG_KEY_PATH(Dir), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(Dir)

			// ListenPort
			SETTING_IMP_BEGIN(ListenPort, CONFIG_KEY_PATH(ListenPort), int)
			void Default() override {
				value_ = 21301;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(ListenPort)

			// RpcListenPort
			SETTING_IMP_BEGIN(RpcListenPort, CONFIG_KEY_PATH(RpcListenPort), int)
			void Default() override {
				value_ = 16888;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(RpcListenPort)

			// RpcSecret
			SETTING_IMP_BEGIN(RpcSecret, CONFIG_KEY_PATH(RpcSecret), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(RpcSecret)

			// Split
			SETTING_IMP_BEGIN(Split, CONFIG_KEY_PATH(Split), int)
			void Default() override {
				value_ = 16;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(Split)

			// UserAgent
			SETTING_IMP_BEGIN(UserAgent, CONFIG_KEY_PATH(UserAgent), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(UserAgent)

			// AllProxy
			SETTING_IMP_BEGIN(AllProxy, CONFIG_KEY_PATH(AllProxy), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(AllProxy)

			// DhtListenPort
			SETTING_IMP_BEGIN(DhtListenPort, CONFIG_KEY_PATH(DhtListenPort), int)
			void Default() override {
				value_ = 6881;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(DhtListenPort)

			// MaxConcurrentDownloads
			SETTING_IMP_BEGIN(MaxConcurrentDownloads, CONFIG_KEY_PATH(MaxConcurrentDownloads), int)

			void Default() override {
				value_ = 5;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxConcurrentDownloads)

			//ConfPath
			SETTING_IMP_BEGIN(ConfPath, CONFIG_KEY_PATH(ConfPath), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(ConfPath)

			// TrackerSourceUrls
			SETTING_IMP_BEGIN(TrackerSourceUrls, CONFIG_KEY_PATH(TrackerSourceUrls), QString)
			void Default() override {
				value_ = QString();
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(TrackerSourceUrls)

			//SaveSession
			SETTING_IMP_BEGIN(SaveSession, CONFIG_KEY_PATH(SaveSession), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(SaveSession)

			// IsSaveSession
			SETTING_IMP_BEGIN(IsSaveSession, CONFIG_KEY_PATH(IsSaveSession), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(IsSaveSession)

			//EnableGlobalProxy
			SETTING_IMP_BEGIN(EnableGlobalProxy, CONFIG_KEY_PATH(EnableGlobalProxy), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableGlobalProxy)

			//GlobalProxy
			SETTING_IMP_BEGIN(GlobalProxy, CONFIG_KEY_PATH(GlobalProxy), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(GlobalProxy)

			// ListenClipboard
			SETTING_IMP_BEGIN(ListenClipboard, CONFIG_KEY_PATH(ListenClipboard), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(ListenClipboard)

			// AutoResumeTask
			SETTING_IMP_BEGIN(AutoResumeTask, CONFIG_KEY_PATH(AutoResumeTask), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(AutoResumeTask)

			//AutoStart
			SETTING_IMP_BEGIN(AutoStart, CONFIG_KEY_PATH(AutoStart), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}

			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(AutoStart)

			//RememberWindowPosition
			SETTING_IMP_BEGIN(RememberWindowPosition, CONFIG_KEY_PATH(RememberWindowPosition), bool)
			void Default() override {
				value_ = true;
			}

			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(RememberWindowPosition)

			// WindowPosition
			SETTING_IMP_BEGIN(WindowPosition, CONFIG_KEY_PATH(WindowPosition), QPoint)
			void Default() override {
				value_ = QPoint(0, 0);
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<QPoint>()) {
					value_ = value.toPoint();
				}
				else if (value.canConvert<QString>()) {
					QString str		 = value.toString();
					QStringList list = str.split(",");
					if (list.size() == 2) {
						value_ = QPoint(list[0].toInt(), list[1].toInt());
					}
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString("%1,%2").arg(value_.x()).arg(value_.y());
			}
			SETTING_IMP_END(WindowPosition)

			// EnableTrayIcon
			SETTING_IMP_BEGIN(EnableTrayIcon, CONFIG_KEY_PATH(EnableTrayIcon), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableTrayIcon)

			// EnableNotification
			SETTING_IMP_BEGIN(EnableNotification, CONFIG_KEY_PATH(EnableNotification), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableNotification)

			// EnableAutoShutdown
			SETTING_IMP_BEGIN(EnableAutoShutdown, CONFIG_KEY_PATH(EnableAutoShutdown), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}

			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableAutoShutdown)

			// EnableAutoUpdate
			SETTING_IMP_BEGIN(EnableAutoUpdate, CONFIG_KEY_PATH(EnableAutoUpdate), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableAutoUpdate)

			// EnableGithubAccelerate
			SETTING_IMP_BEGIN(EnableGithubAccelerate, CONFIG_KEY_PATH(EnableGithubAccelerate), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					auto str = value.toString();
					value_   = str == "true" || str == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableGithubAccelerate)

			// BaiduPanCookies
			SETTING_IMP_BEGIN(BaiduPanCookies, CONFIG_KEY_PATH(BaiduPanCookies), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<QString>()) {
					value_ = value.toString();
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(BaiduPanCookies)

			// TrackerSourceNames
			SETTING_IMP_BEGIN(TrackerSourceNames, CONFIG_KEY_PATH(TrackerSourceNames), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<QString>()) {
					value_ = value.toString();
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(TrackerSourceNames)

			//EnableTrackerSourceAutoUpdate
			SETTING_IMP_BEGIN(EnableTrackerSourceAutoUpdate, CONFIG_KEY_PATH(EnableTrackerSourceAutoUpdate), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
            VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
            SETTING_IMP_END(EnableTrackerSourceAutoUpdate)

			// ShowCloseConfirm
			SETTING_IMP_BEGIN(ShowCloseConfirm, CONFIG_KEY_PATH(ShowCloseConfirm), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(ShowCloseConfirm)

			// CloseToTray
			SETTING_IMP_BEGIN(CloseToTray, CONFIG_KEY_PATH(CloseToTray), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(CloseToTray)

			// MaxDownloadLimit (单位: KB/s)
			SETTING_IMP_BEGIN(MaxDownloadLimit, CONFIG_KEY_PATH(MaxDownloadLimit), int)
			void Default() override {
				value_ = 0;  // 0 = unlimited
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxDownloadLimit)

			// MaxOverallDownloadLimit (单位: KB/s)
			SETTING_IMP_BEGIN(MaxOverallDownloadLimit, CONFIG_KEY_PATH(MaxOverallDownloadLimit), int)
			void Default() override {
				value_ = 0;  // 0 = unlimited
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxOverallDownloadLimit)

			// MaxUploadLimit (单位: KB/s)
			SETTING_IMP_BEGIN(MaxUploadLimit, CONFIG_KEY_PATH(MaxUploadLimit), int)
			void Default() override {
				value_ = 0;  // 0 = unlimited
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxUploadLimit)

			// MaxOverallUploadLimit (单位: KB/s)
			SETTING_IMP_BEGIN(MaxOverallUploadLimit, CONFIG_KEY_PATH(MaxOverallUploadLimit), int)
			void Default() override {
				value_ = 0;  // 0 = unlimited
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxOverallUploadLimit)

			// LowestSpeedLimit (单位: KB/s)
			SETTING_IMP_BEGIN(LowestSpeedLimit, CONFIG_KEY_PATH(LowestSpeedLimit), int)
			void Default() override {
				value_ = 0;  // 0 = disabled
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(LowestSpeedLimit)

			// MaxConnectionPerServer (单服务器最大连接数)
			SETTING_IMP_BEGIN(MaxConnectionPerServer, CONFIG_KEY_PATH(MaxConnectionPerServer), int)
			void Default() override {
				value_ = 16;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxConnectionPerServer)

			// MinSplitSize (最小分割大小，单位: MB)
			SETTING_IMP_BEGIN(MinSplitSize, CONFIG_KEY_PATH(MinSplitSize), int)
			void Default() override {
				value_ = 20;  // 20MB
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MinSplitSize)

			// OnCompleteAction (下载完成后操作：0=无操作, 1=打开文件, 2=打开目录, 3=播放声音, 4=自定义命令, 5=关机, 6=睡眠, 7=重启)
			SETTING_IMP_BEGIN(OnCompleteAction, CONFIG_KEY_PATH(OnCompleteAction), int)
			void Default() override {
				value_ = 0;  // 默认：无操作
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(OnCompleteAction)

			// CustomCompleteCommand (自定义完成命令)
			SETTING_IMP_BEGIN(CustomCompleteCommand, CONFIG_KEY_PATH(CustomCompleteCommand), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(CustomCompleteCommand)

			// OnErrorAction (下载错误后操作：0=无操作, 1=播放声音, 2=自定义命令)
			SETTING_IMP_BEGIN(OnErrorAction, CONFIG_KEY_PATH(OnErrorAction), int)
			void Default() override {
				value_ = 0;  // 默认：无操作
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(OnErrorAction)

			// CustomErrorCommand (自定义错误命令)
			SETTING_IMP_BEGIN(CustomErrorCommand, CONFIG_KEY_PATH(CustomErrorCommand), QString)
			void Default() override {
				value_ = "";
			}
			void Put(const QVariant& value) override {
				value_ = value.toString();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_;
			}
			SETTING_IMP_END(CustomErrorCommand)

			// OnStartAction (下载开始后操作：0=无操作, 1=播放声音)
			SETTING_IMP_BEGIN(OnStartAction, CONFIG_KEY_PATH(OnStartAction), int)
			void Default() override {
				value_ = 0;  // 默认：无操作
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(OnStartAction)

			// Timeout (连接超时，单位：秒)
			SETTING_IMP_BEGIN(Timeout, CONFIG_KEY_PATH(Timeout), int)
			void Default() override {
				value_ = 60;  // 60秒
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(Timeout)

			// ConnectTimeout (建立连接超时，单位：秒)
			SETTING_IMP_BEGIN(ConnectTimeout, CONFIG_KEY_PATH(ConnectTimeout), int)
			void Default() override {
				value_ = 60;  // 60秒
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(ConnectTimeout)

			// MaxTries (最大重试次数，0=无限)
			SETTING_IMP_BEGIN(MaxTries, CONFIG_KEY_PATH(MaxTries), int)
			void Default() override {
				value_ = 5;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(MaxTries)

			// RetryWait (重试等待时间，单位：秒，0=禁用)
			SETTING_IMP_BEGIN(RetryWait, CONFIG_KEY_PATH(RetryWait), int)
			void Default() override {
				value_ = 0;  // 默认禁用
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(RetryWait)

			// EnableDht (启用 DHT)
			SETTING_IMP_BEGIN(EnableDht, CONFIG_KEY_PATH(EnableDht), bool)
			void Default() override {
				value_ = true;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(EnableDht)

			// BtMaxPeers (BT 最大对等节点数)
			SETTING_IMP_BEGIN(BtMaxPeers, CONFIG_KEY_PATH(BtMaxPeers), int)
			void Default() override {
				value_ = 55;
			}
			void Put(const QVariant& value) override {
				value_ = value.toInt();
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return QString::number(value_);
			}
			SETTING_IMP_END(BtMaxPeers)

			// BtRequireCrypto (BT 要求加密)
			SETTING_IMP_BEGIN(BtRequireCrypto, CONFIG_KEY_PATH(BtRequireCrypto), bool)
			void Default() override {
				value_ = false;
			}
			void Put(const QVariant& value) override {
				if (value.canConvert<bool>()) {
					value_ = value.toBool();
				}
				else if (value.canConvert<QString>()) {
					value_ = value.toString() == "true" || value.toString() == "1";
				}
				else if (value.canConvert<int>()) {
					value_ = value.toInt() == 1;
				}
			}
			VALUE_TYPE Get() const {
				return value_;
			}
			QString ToString() override {
				return value_ ? "true" : "false";
			}
			SETTING_IMP_END(BtRequireCrypto)

		}  // namespace settings
	}  // namespace ui
}  // namespace gdl

