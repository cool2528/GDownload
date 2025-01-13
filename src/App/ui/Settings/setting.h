#pragma once
#include <qvariant.h>
#include <QObject>
#include <QSize>
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
        explicit CLASS_NAME() : Setting(KEY) {}       \
        using VALUE_TYPE					  = TYPE; \
        inline static const char* setting_key = KEY;  \
                                                      \
       protected:                                     \
        VALUE_TYPE value_;                            \
                                                      \
       public:

#define SETTING_IMP_END() \
    }                     \
    ;

            // WindowSize
            SETTING_IMP_BEGIN(WindowSize, "general.window_size", QSize)
            void Default() override {
                value_ = QSize(1024, 768);
            }
            void Put(const QVariant& value) override {
                value_ = value.toSize();
            }
            VALUE_TYPE Get() const {
                return value_;
            }
            QString ToString() override {
                return QString("%1,%2").arg(value_.width()).arg(value_.height());
            }
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

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
            SETTING_IMP_END()

            // TrackerSourceUrls
            SETTING_IMP_BEGIN(TrackerSourceUrls, "aria2c.tracker-source-urls", QStringList)
            void Default() override {
                value_ = QStringList();
            }
            void Put(const QVariant& value) override {
                value_ = value.toStringList();
            }
            VALUE_TYPE Get() const {
                return value_;
            }
            QString ToString() override {
                return value_.join(";");
            }
            SETTING_IMP_END()

        }  // namespace settings
    }  // namespace ui
}  // namespace gdl
