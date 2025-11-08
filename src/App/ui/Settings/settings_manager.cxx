#include "settings_manager.h"
#include <QQmlEngine>
#include "Definitions/appDef.h"
#include "config/config.h"
#include "Aria2CManager/aria2c_manager.h"
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
                Save();
                return true;
            }

            void Settings::UnInit() {
                Save();
            }
            Settings::Settings(QObject* parent) {}

            void Settings::SetAria2GlobalProxy(const QString &proxy)
            {
                SetGlobalProxy(proxy);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"all-proxy", proxy.toStdString()});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2AutoResumeTask(bool enable)
            {
                SetAutoResumeTask(enable);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"continue", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MaxDownloadLimit(int value)
            {
                SetMaxDownloadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-download-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MaxOverallDownloadLimit(int value)
            {
                SetMaxOverallDownloadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-overall-download-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MaxUploadLimit(int value)
            {
                SetMaxUploadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-upload-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MaxOverallUploadLimit(int value)
            {
                SetMaxOverallUploadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-overall-upload-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2LowestSpeedLimit(int value)
            {
                SetLowestSpeedLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"lowest-speed-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

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
