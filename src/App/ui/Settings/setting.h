#pragma once
#include <qvariant.h>
#include <QHash>
#include <QObject>
#include <QRect>
#include <QSize>
#include <nlohmann/json.hpp>
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
			SETTING_IMP_BEGIN(WindowSize, "general.window_size", QSize)
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
			SETTING_IMP_BEGIN(Theme, "general.theme", QString)
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
			SETTING_IMP_BEGIN(Language, "general.language", QString)
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
			SETTING_IMP_BEGIN(BtExludeTracker, "aria2c.bt-exclude-tracker", QString)
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
			SETTING_IMP_BEGIN(BtTracker, "aria2c.bt-tracker", QString)
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
			SETTING_IMP_BEGIN(Dir, "aria2c.dir", QString)
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
			SETTING_IMP_BEGIN(ListenPort, "aria2c.listen-port", int)
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
			SETTING_IMP_BEGIN(RpcListenPort, "aria2c.rpc-listen-port", int)
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

			// Split
			SETTING_IMP_BEGIN(Split, "aria2c.split", int)
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
			SETTING_IMP_BEGIN(UserAgent, "aria2c.user-agent", QString)
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
			SETTING_IMP_BEGIN(AllProxy, "aria2c.all-proxy", QString)
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
			SETTING_IMP_BEGIN(DhtListenPort, "aria2c.dht-listen-port", int)
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
			SETTING_IMP_BEGIN(MaxConcurrentDownloads, "aria2c.max-concurrent-downloads", int)

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
			SETTING_IMP_BEGIN(ConfPath, "aria2c.conf-path", QString)
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
			SETTING_IMP_BEGIN(TrackerSourceUrls, "aria2c.tracker_source_urls", QString)
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
			SETTING_IMP_BEGIN(SaveSession, "aria2c.save-session", QString)
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
			SETTING_IMP_BEGIN(IsSaveSession, "aria2c.is-save-session", bool)
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
			SETTING_IMP_BEGIN(EnableGlobalProxy, "aria2c.enable-global-proxy", bool)
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
			SETTING_IMP_BEGIN(GlobalProxy, "aria2c.global-proxy", QString)
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
			SETTING_IMP_BEGIN(ListenClipboard, "general.listen-clipboard", bool)
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
			SETTING_IMP_BEGIN(AutoResumeTask, "aria2c.auto-resume-task", bool)
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
			SETTING_IMP_BEGIN(AutoStart, "general.auto-start", bool)
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
			SETTING_IMP_BEGIN(RememberWindowPosition, "general.remember-window-position", bool)
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
			SETTING_IMP_BEGIN(WindowPosition, "general.window-position", QPoint)
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
			SETTING_IMP_BEGIN(EnableTrayIcon, "general.enable-tray-icon", bool)
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
			SETTING_IMP_BEGIN(EnableNotification, "general.enable-notification", bool)
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
			SETTING_IMP_BEGIN(EnableAutoShutdown, "general.enable-auto-shutdown", bool)
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
			SETTING_IMP_BEGIN(EnableAutoUpdate, "general.enable-auto-update", bool)
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

			// BaiduPanCookies
			SETTING_IMP_BEGIN(BaiduPanCookies, "plugin.baidu-pan-cookies", QString)
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
			SETTING_IMP_BEGIN(TrackerSourceNames, "aria2c.tracker_source_name", QString)
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
			SETTING_IMP_BEGIN(EnableTrackerSourceAutoUpdate, "aria2c.enable_tracker_source_auto_update", bool)
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

		}  // namespace settings
	}  // namespace ui
}  // namespace gdl
