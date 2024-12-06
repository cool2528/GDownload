#include "browser_manager.h"
#include <QApplication>
#include <QQmlEngine>
#include <nlohmann/json.hpp>
#include "Aria2CManager/engine_def.h"
#include "Definitions/appDef.h"
#include "logger.h"
namespace gdl {
	namespace ui {
		namespace browser {

			BrowserManager* BrowserManager::create(QQmlEngine* qmlengine, QJSEngine* jsengine) {
				Q_UNUSED(qmlengine)
				Q_UNUSED(jsengine);
				return &BrowserManager::Instance();
			}

			BrowserManager::~BrowserManager() {}

			DownloadTaskModel* BrowserManager::GetActiveDownloadModel() {
				return active_model_.get();
			}

			DownloadTaskModel* BrowserManager::GetStopedDownloadModel() {
				return stoped_model_.get();
			}

			DownloadTaskModel* BrowserManager::GetWaitingDownloadModel() {
				return waiting_model_.get();
			}

			bool BrowserManager::AddHttpTask(const QString& url, const QVariantMap& options) {
				std::unordered_multimap<std::string, std::string> opt;
				for (auto it = options.cbegin(); it != options.cend(); ++it) {
					auto key   = it.key();
					auto value = it.value().toString();
					opt.emplace(key.toStdString(), value.toStdString());
				}
				auto res = engine::Aria2cDownloadManager::Instance().AddHttpTask(url.toStdString(), opt);
				return res.IsOk();
			}

			bool BrowserManager::AddTorrentTask(const QString& tarrent, const QVariantMap& options) {
				return false;
			}

			bool BrowserManager::AddMetalinkTask(const QString& metalink, const QVariantMap& options) {
				return false;
			}

			bool BrowserManager::Init() {
				auto res = engine::Aria2cDownloadManager::Instance().SubscriptionAria2Message(
					kAria2Responce, [this](const std::string& msg) { OnHandleAria2Message(msg); });
				if (res.HasError()) return false;
				subcription_ = res.Value();
				return true;
			}

			void BrowserManager::UnInit() {
				if (subcription_) {
					engine::Aria2cDownloadManager::Instance().UnSubscribeAria2Message(subcription_);
				}
			}

			BrowserManager::BrowserManager(QObject* parent) : QObject(parent) {
				active_model_  = std::make_unique<DownloadTaskModel>();
				waiting_model_ = std::make_unique<DownloadTaskModel>();
				stoped_model_  = std::make_unique<DownloadTaskModel>();
			}

			void BrowserManager::OnHandleAria2Message(const std::string& msg) {
				try {
					LOG_DBG("OnHandleAria2Message {}", msg);
				} catch (std::exception& e) {
					LOG_ERR("{}", e.what());
				} catch (...) {
					LOG_ERR("OnHandleAria2Message exception");
				}
			}

			void RegisterTypes(QQmlEngine* engine) {
				qmlRegisterSingletonInstance<BrowserManager>(GEXPORT_MODULE_URL, 1, 0, "BrowserManager",
															 &BrowserManager::Instance());
				const QString app_path = QCoreApplication::applicationDirPath();
				QString aria2c_engine_path;
#ifdef __APPLE__
				QString app_path_dir =
					QString::fromStdString(std::filesystem::path(app_path.toStdString()).parent_path().string());
				aria2c_engine_path = app_path_dir + "/Resources/engine/aria2c";
#elif _WIN32 || defined(_WIN64)
				aria2c_engine_path = app_path + "/engine/aria2c.exe";
#else
				aria2c_engine_path = app_path + "/engine/aria2c";
#endif
				gdl::engine::Aria2cDownloadManager::Instance().InitAria2cEngine(aria2c_engine_path.toStdString());
				BrowserManager::Instance().Init();
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
