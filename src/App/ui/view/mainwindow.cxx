#include "mainwindow.h"
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlContext>
#include <QUrl>
#include "Browser/browser_manager.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"
#include "FramelessHelper/Core/private/framelessconfig_p.h"
#include "FramelessHelper/Quick/framelessquickmodule.h"
#include "GDLCore/logger.h"
#include "theme/theme.h"
#include "utils/utils.h"
FRAMELESSHELPER_USE_NAMESPACE
namespace gd {
	namespace ui {
		MainWindow::MainWindow(QObject* parent) : QObject(parent) {}
		MainWindow::~MainWindow() {}
		int MainWindow::Exec(int argc, char* argv[]) {
			FramelessHelper::Quick::initialize();
			QGuiApplication app(argc, argv);
#if defined(_WIN32) || defined(_WIN64)
            app.setWindowIcon(QIcon(":/images/logo/icon.ico"));
#endif
			QQmlApplicationEngine engine;
			InitQmlEngine(&engine);
			InitFont(&engine);
			QObject::connect(
				&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
				Qt::QueuedConnection);
			const QUrl url(QStringLiteral("qrc:/qml/mainWindow.qml"));
			engine.addImportPath(QStringLiteral("qrc:/qml"));
			engine.load(url);
			const auto code = app.exec();
			UnInitEngine();
			return code;
		}

		void MainWindow::InitQmlEngine(QQmlEngine* engine) {
			FramelessHelper::Core::setApplicationOSThemeAware();
			FramelessHelper::Quick::registerTypes(engine);
			gdl::ui::utils::RegisterTypes(engine);
			gdl::ui::theme::RegisterTypes(engine);
			qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
											 "SegoeFluentIcons", "SegoeFluentIcons enum");
			gdl::ui::browser::RegisterTypes(engine);
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

		void MainWindow::InitIcon(QGuiApplication* app) {}

		void MainWindow::UnInitEngine() {
			gdl::ui::browser::BrowserManager::Instance().UnInit();
			gdl::engine::Aria2cDownloadManager::Instance().UninitAria2cEngine();
		}

	}  // namespace ui
}  // namespace gd
