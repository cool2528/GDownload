#include "mainwindow.h"
#include <QApplication>
#include <QUrl>
#include "FramelessHelper/Core/private/framelessconfig_p.h"
#include "FramelessHelper/Quick/framelessquickmodule.h"
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
        }
        void MainWindow::InitTranslation(QGuiApplication* app)
        {

        }
        void MainWindow::InitFont(QGuiApplication* app)
        {

        }
        
        void MainWindow::InitIcon(QGuiApplication* app)
        {

        }
    
    }
}
