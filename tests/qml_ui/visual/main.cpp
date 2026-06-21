#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>

#include "ScreenshotHelper.h"

// QuickTest setup:QML 引擎就绪时注入 Screenshot 上下文属性
// 视觉用例通过 Screenshot.capture(item, "tag") 触发截图(Phase 3 填实)
class TestSetup : public QObject {
    Q_OBJECT

   public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
        // ScreenshotHelper 父对象设为 engine,引擎销毁时自动释放
        auto* helper = new gdl::tests::ScreenshotHelper(engine);
        engine->rootContext()->setContextProperty("Screenshot", helper);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qml_ui_visual, TestSetup)

#include "main.moc"
