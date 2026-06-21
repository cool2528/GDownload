#include "settings_manager.h"
#include <unordered_map>
#include <QQmlEngine>
#include <QRandomGenerator>
#include "Definitions/appDef.h"
#include "config/config.h"
#include "Aria2CManager/aria2c_manager.h"
namespace gdl {
    namespace ui {
        namespace settings {

            SettingsImpl::~SettingsImpl() {}

            Settings* SettingsImpl::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
                Q_UNUSED(qmlengine)
                Q_UNUSED(jsengine);
                return &SettingsImpl::Instance();
            }

            bool SettingsImpl::Init() {
                QHashIterator<QString, Setting*> i(Setting::settings_);
                while (i.hasNext()) {
                    i.next();
                    QString key		  = i.key();
                    auto setting	  = i.value();
                    std::string value = config::GetValue(key.toStdString()).AsString();
                    if (value.empty()) {
                        // 配置缺失（首次运行或配置损坏）时使用各 Setting 自身的默认值，
                        // 否则端口、主题、并发数等会得到空值/零值而非 Default() 定义的值。
                        setting->Default();
                    } else {
                        setting->Put(QString::fromStdString(value));
                    }
                }
                Save();
                return true;
            }

            void SettingsImpl::UnInit() {
                Save();
            }
            SettingsImpl::SettingsImpl(QObject* parent) : QObject(parent) {}

            QString SettingsImpl::GenerateRpcSecret() const
            {
                static constexpr char kChars[] =
                    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
                QString secret;
                secret.reserve(32);
                auto* generator = QRandomGenerator::system();
                for (int i = 0; i < 32; ++i) {
                    secret.append(QLatin1Char(kChars[generator->bounded(static_cast<int>(sizeof(kChars) - 1))]));
                }
                return secret;
            }

            void SettingsImpl::SetAria2Dir(const QString& dir)
            {
                SetDir(dir);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"dir", dir.toStdString()});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2GlobalProxy(const QString &proxy)
            {
                SetGlobalProxy(proxy);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"all-proxy", proxy.toStdString()});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2AutoResumeTask(bool enable)
            {
                SetAutoResumeTask(enable);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"continue", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxDownloadLimit(int value)
            {
                SetMaxDownloadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-download-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxOverallDownloadLimit(int value)
            {
                SetMaxOverallDownloadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-overall-download-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxUploadLimit(int value)
            {
                SetMaxUploadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-upload-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxOverallUploadLimit(int value)
            {
                SetMaxOverallUploadLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-overall-upload-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2LowestSpeedLimit(int value)
            {
                SetLowestSpeedLimit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"lowest-speed-limit", std::to_string(value * 1024)});  // KB -> Bytes
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxConcurrentDownloads(int value)
            {
                SetMaxConcurrentDownloads(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-concurrent-downloads", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2Split(int value)
            {
                SetSplit(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"split", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MaxConnectionPerServer(int value)
            {
                SetMaxConnectionPerServer(value);
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-connection-per-server", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2MinSplitSize(int sizeMB)
            {
                SetMinSplitSize(sizeMB);
                // 转换为字节（MB -> Bytes）并添加 "M" 后缀
                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"min-split-size", std::to_string(sizeMB) + "M"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(engine::Aria2Method::kChangeGlobalOption,opt);
            }

            void SettingsImpl::SetAria2Timeout(int value)
            {
                SetTimeout(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"timeout", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2ConnectTimeout(int value)
            {
                SetConnectTimeout(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"connect-timeout", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2MaxTries(int value)
            {
                SetMaxTries(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"max-tries", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2RetryWait(int value)
            {
                SetRetryWait(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"retry-wait", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2EnableDht(bool enable)
            {
                SetEnableDht(enable);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"enable-dht", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2BtMaxPeers(int value)
            {
                SetBtMaxPeers(value);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"bt-max-peers", std::to_string(value)});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2BtRequireCrypto(bool enable)
            {
                SetBtRequireCrypto(enable);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"bt-require-crypto", enable ? "true" : "false"});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::SetAria2UserAgent(const QString& userAgent)
            {
                SetUserAgent(userAgent);

                std::unordered_multimap<std::string, std::string> opt;
                opt.insert({"user-agent", userAgent.toStdString()});
                engine::Aria2cDownloadManager::Instance().CallAria2cMethod(
                    engine::Aria2Method::kChangeGlobalOption, opt);
            }

            void SettingsImpl::Save() {
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
                Q_UNUSED(engine);
                gdl::ui::settings::SettingsImpl::Instance().Init();
                // 单例注册已迁移至 MainWindow::InitQmlEngine,经由 createSettingsManager 工厂注入
            }

        }  // namespace settings
    }  // namespace ui
}  // namespace gdl
