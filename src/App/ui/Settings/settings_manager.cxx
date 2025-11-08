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

            void Settings::SetAria2MaxConcurrentDownloads(int value)
            {
                SetMaxConcurrentDownloads(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-concurrent-downloads", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2Split(int value)
            {
                SetSplit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"split", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MaxConnectionPerServer(int value)
            {
                SetMaxConnectionPerServer(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-connection-per-server", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2MinSplitSize(int sizeMB)
            {
                SetMinSplitSize(sizeMB);
                // 转换为字节（MB -> Bytes）并添加 "M" 后缀
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"min-split-size", std::to_string(sizeMB) + "M"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void Settings::SetAria2Timeout(int value)
            {
                SetTimeout(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"timeout", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2ConnectTimeout(int value)
            {
                SetConnectTimeout(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"connect-timeout", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2MaxTries(int value)
            {
                SetMaxTries(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-tries", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2RetryWait(int value)
            {
                SetRetryWait(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"retry-wait", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2EnableDht(bool enable)
            {
                SetEnableDht(enable);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"enable-dht", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2BtMaxPeers(int value)
            {
                SetBtMaxPeers(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"bt-max-peers", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2BtRequireCrypto(bool enable)
            {
                SetBtRequireCrypto(enable);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"bt-require-crypto", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void Settings::SetAria2UserAgent(const QString& userAgent)
            {
                SetUserAgent(userAgent);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"user-agent", userAgent.toStdString()});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
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
