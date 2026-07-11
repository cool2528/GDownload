#include "mainwindow.h"
#include <QApplication>
#include "engine_startup_policy.h"
#include <QFontDatabase>
#include <QQmlContext>
#include <QUrl>
#include <filesystem>
#include "Browser/browser_manager.h"
#include "Browser/browser_manager_factory.h"
#include "test_mode.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"
#include "FramelessHelper/Core/private/framelessconfig_p.h"
#include "FramelessHelper/Quick/framelessquickmodule.h"
#include "GDLCore/logger.h"
#include "Models/folder_history_model.h"
#include "NetDisk/NetWork_Disk_magager.h"
#include "PluginManager/plugin_manager.h"
#include "Settings/settings_manager.h"
#include "Settings/settings_manager_factory.h"
#include "language/language_manager.h"
#include "logger.h"
#include "os/os.h"
#include "theme/theme.h"
#include "toast/toast_manager.h"
#include "update/update_manager.h"
#include "utils/utils.h"
#include "version.h"
FRAMELESSHELPER_USE_NAMESPACE
namespace gd {
	namespace ui {
		MainWindow::MainWindow(QObject* parent) : QObject(parent) {
			InitQtMessageHandler();
		}
		MainWindow::~MainWindow() {}
		int MainWindow::Exec(int argc, char* argv[]) {

			FramelessHelper::Quick::initialize();
			QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
			QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
			QApplication::setOrganizationName("gdl");
			QApplication::setApplicationName("gdownload");
			QApplication app(argc, argv);
			InitIcon(&app);
			QQmlApplicationEngine engine;
			if (!gdl::ui::isTestMode()) {
				InitNetDiskPlugins();
			}
			InitQmlEngine(&engine);
			InitFont(&engine);
			QObject::connect(
				&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QApplication::exit(-1); },
				Qt::QueuedConnection);
			const QUrl url(QStringLiteral("qrc:/qml/mainWindow.qml"));
			engine.addImportPath(QStringLiteral("qrc:/qml"));
			engine.load(url);
			const auto code = app.exec();
			UnInitEngine();
			return code;
		}

		void MainWindow::InitQmlEngine(QQmlEngine* engine) {
			const bool is_test = gdl::ui::isTestMode();
			// 经工厂创建 manager 并注册为 QML 单例
			// 模板参数用 Impl 类型:IBrowserManager/ISettings 未继承 QObject,不能作为
			// qmlRegisterSingletonInstance 的模板实参;Phase 2 若 Fake 继承 Impl 则无需改此行
			auto* settings_ptr = gdl::ui::settings::createSettingsManager(is_test);
			auto* browser_ptr  = gdl::ui::browser::createBrowserManager(is_test);
			qmlRegisterSingletonInstance<gdl::ui::settings::SettingsImpl>(
				GEXPORT_MODULE_URL, 1, 0, "SettingsManager",
				static_cast<gdl::ui::settings::SettingsImpl*>(settings_ptr));
			qmlRegisterSingletonInstance<gdl::ui::browser::BrowserManagerImpl>(
				GEXPORT_MODULE_URL, 1, 0, "BrowserManager",
				static_cast<gdl::ui::browser::BrowserManagerImpl*>(browser_ptr));
			gdl::ui::settings::RegisterTypes(engine);
			qmlRegisterType<gdl::ui::models::FolderHistoryModel>("gdl.sdk", 1, 0, "FolderHistoryModel");
			gdl::ui::language::RegisterTypes(engine);
			FramelessHelper::Core::setApplicationOSThemeAware();
			FramelessHelper::Quick::registerTypes(engine);
			gdl::ui::utils::RegisterTypes(engine);
			gdl::ui::theme::RegisterTypes(engine);
			qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
											 "SegoeFluentIcons", "SegoeFluentIcons enum");
			const auto cache_init_result = gdl::cache::DownloadHistoryCache::Instance().Initialize(
				gdl::os::GetAppDataDir() + "/gdownload/db/gdownload.db");
			if (cache_init_result.HasError()) {
				LOG_ERR("init download history cache fail: {}", cache_init_result.GetError().Describe())
			}
			gdl::ui::browser::RegisterTypes(engine);
			gdl::ui::toast::RegisterTypes(engine);
			gdl::update::RegisterTypes(engine);
			gdl::ui::netdisk::RegisterTypes(engine);
			// 启动期副作用:测试模式下跳过 aria2c 子进程、自动更新 HTTP
			if (!is_test) {
				// aria2c 引擎初始化(原 browser_manager.cxx RegisterTypes 内的逻辑)
				const QString app_path = QString::fromStdString(gdl::os::GetExecutableDir());
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
				auto& browser_manager = gdl::ui::browser::BrowserManagerImpl::Instance();
				gdl::engine::Aria2cDownloadManager::Instance().SetEngineAvailabilityCallback(
					[manager = &browser_manager](bool available) {
						if (available) return;
						QMetaObject::invokeMethod(manager, [manager] {
							manager->SetEngineUnavailable(QObject::tr("The download engine stopped unexpectedly."));
						}, Qt::QueuedConnection);
					});
				gdl::ui::RunEngineStartupPolicy(
					[&] { return gdl::engine::Aria2cDownloadManager::Instance().InitAria2cEngine(
						aria2c_engine_path.toStdString()); },
					[&] { return browser_manager.Init(); },
					[&] { gdl::engine::Aria2cDownloadManager::Instance().UninitAria2cEngine(); },
					[&] { browser_manager.SetEngineUnavailable(QObject::tr("Download engine is unavailable.")); });
				// 自动更新检查
				gdl::update::UpdateConfig update_config;
				update_config.current_version	 = GDownload_VERSION_STRING;
				update_config.enable_auto_check = gdl::ui::settings::Settings::Instance().GetEnableAutoUpdate();
				update_config.update_url = "https://gdownload.uk/update/latest.json";
				update_config.fallback_update_url = "https://api.github.com/repos/cool2528/gdownload/releases/latest";
				std::map<std::string, std::string> headers;
				headers["Content-Type"] = "application/json";
				headers["User-Agent"]	= "GDownloader-Update-Client";
				headers["Accept"]				= "application/vnd.github.v3+json";
				gdl::update::UpdateManager::Instance().SetRequestHeaders(headers);
				gdl::update::UpdateManager::Instance().Initialize(update_config);
			}
		}
		void MainWindow::InitTranslation(QGuiApplication* app) {}
		void MainWindow::InitFont(QQmlEngine* engine) {
			fluent_icons_font_id_ = QFontDatabase::addApplicationFont("://font/SegoeFluentIcons.ttf");
			if (fluent_icons_font_id_ == -1) {
				LOG_ERR("init fluenticons font fail");
				return;
			}
			const auto font_family_name = QFontDatabase::applicationFontFamilies(fluent_icons_font_id_).at(0);
			engine->rootContext()->setContextProperty("FluentIcons", font_family_name);
		}

		void MainWindow::InitIcon(QGuiApplication* app) {
#if defined(_WIN32) || defined(_WIN64)
			app->setWindowIcon(QIcon(":/images/logo/icon.ico"));
#endif
		}

		void MainWindow::UnInitEngine() {
			// 与 Init 侧对称:测试模式下未启动 aria2c,跳过 uninit 避免对未初始化状态调用
			const bool is_test = gdl::ui::isTestMode();
			gdl::ui::browser::BrowserManager::Instance().UnInit();
			if (!is_test) {
				gdl::engine::Aria2cDownloadManager::Instance().SetEngineAvailabilityCallback({});
				gdl::engine::Aria2cDownloadManager::Instance().UninitAria2cEngine();
			}
			const auto cache_uninit_result = gdl::cache::DownloadHistoryCache::Instance().Uninitialize();
			if (cache_uninit_result.HasError()) {
				LOG_ERR("uninit download history cache fail: {}", cache_uninit_result.GetError().Describe())
			}
			gdl::ui::settings::Settings::Instance().UnInit();
		}

        void MainWindow::InitNetDiskPlugins() {
            auto current_path = QCoreApplication::applicationDirPath().toStdString();
            auto res		  = gdl::plugin::DownloadPluginManager::Instance().LoadPlugins(current_path);
            if (!res) {
                LOG_ERR("load plugins fail")
            }
        }

		void MainWindow::InitQtMessageHandler() const {
			qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
				QByteArray localMsg	 = msg.toLocal8Bit();
				const char* file	 = context.file ? context.file : "";
				const char* function = context.function ? context.function : "";
				switch (type) {
					case QtDebugMsg:
#if defined(_DEBUG)
						LOG_DBG("Debug: {} ({}, {}, {})", localMsg.constData(), file, context.line, function)
#endif
						break;
					case QtInfoMsg:
						LOG_INFO("Info: {} ({}, {}, {})", localMsg.constData(), file, context.line, function)
						break;
					case QtWarningMsg:
						LOG_WARN("Warning: {} ({}, {}, {})", localMsg.constData(), file, context.line, function)
						break;
					case QtCriticalMsg:
						LOG_CRIT("Critical: {} ({}, {}, {})", localMsg.constData(), file, context.line, function)
						break;
					case QtFatalMsg:
						LOG_ERR("Fatal: {} ({}, {}, {})", localMsg.constData(), file, context.line, function)
						break;
				}
			});
		}

	}  // namespace ui
}  // namespace gd
