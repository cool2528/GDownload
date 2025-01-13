#include "settings_manager.h"
#include <QQmlEngine>
#include "Definitions/appDef.h"
#include "config/config.h"
namespace gdl {
    namespace ui {
        namespace settings {

            Settings::~Settings() {}

            Settings* Settings::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
                Q_UNUSED(qmlengine)
                Q_UNUSED(jsengine);
                return &Settings::Instance();
            }

            bool Settings::Init() {
                QHashIterator<QString, Setting*> i(Setting::settings_);
                while (i.hasNext()) {
                    i.next();
                    QString key		  = i.key();
                    auto setting	  = i.value();
                    std::string value = config::GetValue(key.toStdString()).AsString();
                    setting->Put(QString::fromStdString(value));
                }
                return true;
            }

            void Settings::UnInit() {
                Save();
            }
            Settings::Settings(QObject* parent) {}

            void Settings::Save() {
                QHashIterator<QString, Setting*> i(Setting::settings_);
                while (i.hasNext()) {
                    i.next();
                    QString key	  = i.key();
                    auto setting  = i.value();
                    QString value = setting->ToString();
                    config::SetValue(key.toStdString(), value.toStdString());
                }
            }

            void RegisterTypes(QQmlEngine* engine) {
                gdl::ui::settings::Settings::Instance().Init();
                qmlRegisterSingletonInstance<Settings>(GEXPORT_MODULE_URL, 1, 0, "SettingsManager",
                                                       &Settings::Instance());
            }

        }  // namespace settings
    }  // namespace ui
}  // namespace gdl
