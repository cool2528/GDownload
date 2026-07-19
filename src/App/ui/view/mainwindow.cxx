#include "mainwindow.h"
#include <QApplication>
#include "engine_startup_policy.h"
#include <QCommandLineParser>
#include <QQmlContext>
#include <QStandardPaths>
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
#include "PluginMarket/plugin_config_manager.h"
#include "PluginMarket/plugin_market_manager.h"
#include "Settings/settings_manager.h"
#include "Settings/settings_manager_factory.h"
#include "language/language_manager.h"
#include "logger.h"
#include "os/os.h"
#include "theme/theme.h"
#include "toast/toast_manager.h"
#include "update/update_manager.h"
#include "utils/native_host_registrar.h"
#include "utils/single_instance.h"
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

			// 命令行解析：--silent 静默启动到托盘；位置参数为网盘分享 URL（供浏览器扩展交接）
			QCommandLineParser cmd_parser;
			const QCommandLineOption silent_option(QStringLiteral("silent"),
												   QStringLiteral("Start minimized to system tray"));
			cmd_parser.addOption(silent_option);
			cmd_parser.addPositionalArgument(QStringLiteral("url"), QStringLiteral("Netdisk share URL to open"));
			cmd_parser.parse(app.arguments());  // 容错解析，不用 process 以免未知项时退出
			const bool start_silent = cmd_parser.isSet(silent_option);
			const QStringList positional_args = cmd_parser.positionalArguments();
			const QString open_url = positional_args.isEmpty() ? QString() : positional_args.first();

			// 单实例：已有实例在运行则把参数转发给它后退出，避免多开
			gd::ui::SingleInstanceGuard instance_guard(QStringLiteral("gdownload-single-instance"));
			if (!instance_guard.tryBecomePrimary()) {
				QStringList forward_args;
				if (start_silent) forward_args << QStringLiteral("--silent");
				if (!open_url.isEmpty()) forward_args << open_url;
				instance_guard.sendToPrimary(forward_args);
				LOG_INFO("another GDownload instance is running, forwarded args and exiting");
				return 0;
			}
			// 主实例：收到后续实例转发的参数 -> 提升窗口 + 打开网盘解析页
			QObject::connect(&instance_guard, &gd::ui::SingleInstanceGuard::messageReceived, &app,
							 [](const QStringList& args) {
								 QString url;
								 for (const QString& a : args) {
									 if (!a.startsWith(QStringLiteral("--"))) {
										 url = a;
										 break;
									 }
								 }
								 gdl::ui::browser::BrowserManagerImpl::Instance().TriggerExternalActivate(url);
							 });

			InitIcon(&app);
			QQmlApplicationEngine engine;
			if (!gdl::ui::isTestMode()) {
				InitNetDiskPlugins();
				// 自注册 Native Messaging host（幂等，per-user，无需管理员），使浏览器扩展可发现并唤起 host
				gd::ui::NativeHostRegistrar::EnsureRegistered();
			}
			InitQmlEngine(&engine);
			// 供 QML 读取：是否静默启动到托盘（--silent）
			engine.rootContext()->setContextProperty(QStringLiteral("gAppStartSilent"), start_silent);
			QObject::connect(
				&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QApplication::exit(-1); },
				Qt::QueuedConnection);
			const QUrl url(QStringLiteral("qrc:/qml/mainWindow.qml"));
			engine.addImportPath(QStringLiteral("qrc:/qml"));
			engine.load(url);
			// 本实例启动时携带的网盘 URL：入队等事件循环+QML 就绪后触发
			if (!open_url.isEmpty()) {
				QMetaObject::invokeMethod(
					&app,
					[open_url]() { gdl::ui::browser::BrowserManagerImpl::Instance().TriggerExternalActivate(open_url); },
					Qt::QueuedConnection);
			}
			const auto code = app.exec();
			// 事件循环结束后先销毁 QML 根对象，取消 ListView 等仍在孵化的委托。
			// Managers 必须保持到 QML 对象析构完成，避免析构期绑定访问已反初始化的单例。
			const auto root_objects = engine.rootObjects();
			for (auto* root_object : root_objects) {
				delete root_object;
			}
			engine.collectGarbage();
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
			gdl::ui::market::RegisterTypes(engine);
			qmlRegisterSingletonInstance<gdl::ui::market::PluginMarketManager>(
				GEXPORT_MODULE_URL, 1, 0, "PluginMarketManager",
				&gdl::ui::market::PluginMarketManager::Instance());
			qmlRegisterSingletonInstance<gdl::ui::market::PluginConfigManager>(
				GEXPORT_MODULE_URL, 1, 0, "PluginConfigManager",
				&gdl::ui::market::PluginConfigManager::Instance());
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
#if defined(_WIN32)
				update_config.update_url = "https://gdownload.uk/update/latest-windows-x64.json";
				update_config.fallback_update_url =
					"https://github.com/cool2528/gdownload/releases/latest/download/latest-windows-x64.json";
#elif defined(__linux__)
				update_config.update_url = "https://gdownload.uk/update/latest-linux-x86_64.json";
				update_config.fallback_update_url =
					"https://github.com/cool2528/gdownload/releases/latest/download/latest-linux-x86_64.json";
#endif
				std::map<std::string, std::string> headers;
				headers["Content-Type"] = "application/json";
				headers["User-Agent"]	= "GDownloader-Update-Client";
				headers["Accept"]				= "application/vnd.github.v3+json";
				gdl::update::UpdateManager::Instance().SetRequestHeaders(headers);
				gdl::update::UpdateManager::Instance().Initialize(update_config);
			}
		}
		void MainWindow::InitTranslation(QGuiApplication* app) {}
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
            // JS 脚本插件：<appdir>/plugins/ 下含 manifest.json 的子目录
            // 插件数据（storage/cookies）落在应用数据目录
            auto current_path	= QCoreApplication::applicationDirPath().toStdString();
            auto js_plugins_dir = current_path + "/plugins";
            auto data_dir =
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString();
            auto js_res = gdl::plugin::DownloadPluginManager::Instance().LoadJsPlugins(js_plugins_dir, data_dir);
            if (!js_res) {
                LOG_INFO("no js plugins loaded from {}", js_plugins_dir)
            }
            // 插件市场：注入插件目录与数据目录
            gdl::ui::market::PluginMarketManager::Instance().Initialize(
                QString::fromStdString(js_plugins_dir), QString::fromStdString(data_dir));
            // 插件配置存储 + 旧百度 Cookie 一次性迁移
            gdl::ui::market::PluginConfigManager::Instance().Initialize(QString::fromStdString(data_dir));
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
