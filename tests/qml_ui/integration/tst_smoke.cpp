#include <QtTest>

// Phase 2 smoke:验证 qml_ui_integration_smoke 可执行文件 + offscreen 平台 + QtTest 框架可用
// Phase 4 起追加真实集成用例(tst_save_settings 等,各自独立可执行文件)
class TstSmoke : public QObject {
    Q_OBJECT

   private slots:
    void test_pass() {
        QCOMPARE(1 + 1, 2);
    }
};

QTEST_MAIN(TstSmoke)
#include "tst_smoke.moc"
