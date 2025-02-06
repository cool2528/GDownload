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
#include "Models/folder_history_model.h"
#include "Settings/settings_manager.h"
#include "logger.h"
#include "os/os.h"
#include "theme/theme.h"
#include "utils/utils.h"
FRAMELESSHELPER_USE_NAMESPACE
namespace gd {
    namespace ui {
        MainWindow::MainWindow(QObject* parent) : QObject(parent) {
            InitQtMessageHandler();
        }
        MainWindow::~MainWindow() {}
        int MainWindow::Exec(int argc, char* argv[]) {

            FramelessHelper::Quick::initialize();
            QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
            QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
            QCoreApplication::setOrganizationName("gdl");
            QCoreApplication::setApplicationName("gdownload");
            QGuiApplication app(argc, argv);
            InitIcon(&app);
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
            qmlRegisterType<gdl::ui::models::FolderHistoryModel>("gdl.sdk", 1, 0, "FolderHistoryModel");
            gdl::ui::settings::RegisterTypes(engine);
            FramelessHelper::Core::setApplicationOSThemeAware();
            FramelessHelper::Quick::registerTypes(engine);
            gdl::ui::utils::RegisterTypes(engine);
            gdl::ui::theme::RegisterTypes(engine);
            qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
                                             "SegoeFluentIcons", "SegoeFluentIcons enum");
            if (!gdl::cache::DownloadHistoryCache::Instance().Initialize(gdl::os::GetAppDataDir() +
                                                                         "/gdownload/db/gdownload.db")) {
                LOG_ERR("init download history cache fail")
            }
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

        void MainWindow::InitIcon(QGuiApplication* app) {
#if defined(_WIN32) || defined(_WIN64)
            app->setWindowIcon(QIcon(":/images/logo/icon.ico"));
#endif
        }

        void MainWindow::UnInitEngine() {
            gdl::ui::browser::BrowserManager::Instance().UnInit();
            gdl::engine::Aria2cDownloadManager::Instance().UninitAria2cEngine();
            gdl::cache::DownloadHistoryCache::Instance().Uninitialize();
            gdl::ui::settings::Settings::Instance().UnInit();
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
