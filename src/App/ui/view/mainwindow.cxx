#include "mainwindow.h"
#include <QApplication>
#include <QUrl>
#include <QFontDatabase>
#include <QQmlContext>
#include "FramelessHelper/Core/private/framelessconfig_p.h"
#include "FramelessHelper/Quick/framelessquickmodule.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"
#include "GDLCore/logger.h"
#include "utils/utils.h"
#include "theme/theme.h"
FRAMELESSHELPER_USE_NAMESPACE
namespace gd
{
    namespace ui
    {
        MainWindow::MainWindow(QObject *parent) : QObject(parent)
        {
        }
        MainWindow::~MainWindow()
        {

        }
        int MainWindow::Exec(int argc, char *argv[])
        {
            FramelessHelper::Quick::initialize();
            QGuiApplication app(argc, argv);
            QQmlApplicationEngine engine;
            InitQmlEngine(&engine);
            InitFont(&engine);
            QObject::connect(
                &engine,
                &QQmlApplicationEngine::objectCreationFailed,
                &app,
                []()
                { QCoreApplication::exit(-1); },
                Qt::QueuedConnection);
            const QUrl url(QStringLiteral("qrc:/qml/mainWindow.qml"));
            engine.addImportPath(QStringLiteral("qrc:/qml"));
            engine.load(url);
            return app.exec();
        }

        void MainWindow::InitQmlEngine(QQmlEngine* engine)
        {
            FramelessHelper::Core::setApplicationOSThemeAware();
            FramelessHelper::Quick::registerTypes(engine);
            gdl::ui::utils::RegisterTypes(engine);
            gdl::ui::theme::RegisterTypes(engine);
            qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject,GEXPORT_MODULE_URL,1,0,"SegoeFluentIcons","SegoeFluentIcons enum");

        }
        void MainWindow::InitTranslation(QGuiApplication* app)
        {

        }
        void MainWindow::InitFont(QQmlEngine* engine)
        {
           fluent_icons_font_id_= QFontDatabase::addApplicationFont("://font/SegoeFluentIcons.ttf");
            if(fluent_icons_font_id_ == -1){
               LOG_ERR("init fluenticons font fail");
                return;
            }
            const auto font_family_name = QFontDatabase::applicationFontFamilies(fluent_icons_font_id_).at(0);
            engine->rootContext()->setContextProperty("FluentIcons",font_family_name);

        }
        
        void MainWindow::InitIcon(QGuiApplication* app)
        {

        }
    
    }
}
