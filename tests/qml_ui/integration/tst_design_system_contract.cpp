#include <QtTest>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

class TstDesignSystemContract : public QObject {
    Q_OBJECT

   private slots:
    void no_direct_element_plus_calls_in_qml() {
        const QString root = QStringLiteral("%1/src/App/ui/Resource/qml").arg(QStringLiteral(SOURCE_ROOT));
        QStringList offenders;
        QDirIterator it(root, QStringList() << QStringLiteral("*.qml"), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFile file(path);
            QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));
            const QString text = QString::fromUtf8(file.readAll());
            if (text.contains(QStringLiteral("ElementPlusColors."))) {
                offenders << QDir(root).relativeFilePath(path);
            }
        }
        QVERIFY2(offenders.isEmpty(), qPrintable(QStringLiteral("QML must use GTheme instead of ElementPlusColors: %1").arg(offenders.join(QStringLiteral(", ")))));
    }

    void settings_shell_exposes_design_system_hooks() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/Browser/SettingsPageView.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        QVERIFY2(text.contains(QStringLiteral("objectName: \"settingsShell\"")), "SettingsPageView root must expose objectName settingsShell");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"settingsSidebar\"")), "Settings sidebar must expose objectName settingsSidebar");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"settingsStack\"")), "Settings StackLayout must expose objectName settingsStack");
        QVERIFY2(text.contains(QStringLiteral("GTheme.sidebarWidth")), "Settings shell must use GTheme.sidebarWidth");
        QVERIFY2(!text.contains(QStringLiteral("standardSpacing")), "Settings shell must not keep a local standardSpacing constant");
        QVERIFY2(!text.contains(QStringLiteral("standardRadius")), "Settings shell must not keep a local standardRadius constant");
    }

    void task_dialog_exposes_design_system_hooks() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/CommonComponents/TaskDialogPage.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        QVERIFY2(text.contains(QStringLiteral("objectName: \"taskDialogPage\"")), "TaskDialogPage root must expose objectName taskDialogPage");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"taskDialogTabs\"")), "Task dialog tab strip must expose objectName taskDialogTabs");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"taskDialogStack\"")), "Task dialog StackLayout must expose objectName taskDialogStack");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"taskDialogFooter\"")), "Task dialog footer must expose objectName taskDialogFooter");
    }

    void netdisk_page_exposes_workflow_hooks() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/CommonComponents/NetDiskPageView.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskPage\"")), "NetDiskPageView root must expose objectName netDiskPage");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskWorkflow\"")), "NetDisk workflow area must expose objectName netDiskWorkflow");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskFileList\"")), "NetDisk file list must expose objectName netDiskFileList");
        QVERIFY2(text.contains(QStringLiteral("Paste link")), "NetDisk workflow must expose Paste link copy");
        QVERIFY2(text.contains(QStringLiteral("Preview files")), "NetDisk workflow must expose Preview files copy");
        QVERIFY2(text.contains(QStringLiteral("Add queue")), "NetDisk workflow must expose Add queue copy");
    }

   private:
    QString readFile(const QString& path) const {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // QFAIL 展开为 return;(void 返回),在返回 QString 的函数中触发 MSVC C2561,
            // 故直接调用 QTest::qFail 标记失败并返回空串(语义与 QFAIL 等价)
            QTest::qFail(qPrintable(QStringLiteral("Failed to open %1").arg(path)), __FILE__, __LINE__);
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }
};

QTEST_MAIN(TstDesignSystemContract)
#include "tst_design_system_contract.moc"
