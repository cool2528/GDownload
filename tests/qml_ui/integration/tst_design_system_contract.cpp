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

    void aurora_theme_exposes_semantic_tokens() {
        const QString path = QStringLiteral("%1/src/App/ui/theme/theme.h").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        const QStringList tokens = {
            QStringLiteral("brandHover"),
            QStringLiteral("brandPressed"),
            QStringLiteral("textInverse"),
            QStringLiteral("bgOverlay"),
            QStringLiteral("surfaceBase"),
            QStringLiteral("surfaceElevated"),
            QStringLiteral("focusRing"),
            QStringLiteral("overlayScrim")
        };
        for (const QString& token : tokens) {
            QVERIFY2(text.contains(QStringLiteral("Q_PROPERTY(QColor %1").arg(token)),
                     qPrintable(QStringLiteral("GTheme must expose Aurora token %1").arg(token)));
        }
    }

    void application_shell_uses_aurora_contract() {
        const QString root = QStringLiteral("%1/src/App/ui/Resource/qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString mainWindow = readFile(root + QStringLiteral("/mainWindow.qml"));
        const QString titleBar = readFile(root + QStringLiteral("/titlebar/TitleBar.qml"));
        const QString navigator = readFile(root + QStringLiteral("/Navigator/NavigatorView.qml"));

        QVERIFY2(mainWindow.contains(QStringLiteral("minimumWidth: 900")), "mainWindow must preserve the 900 px Aurora minimum width");
        QVERIFY2(mainWindow.contains(QStringLiteral("minimumHeight: 640")), "mainWindow must preserve the 640 px Aurora minimum height");
        QVERIFY2(mainWindow.contains(QStringLiteral("sectionTitle: brower_view.currentSectionTitle")), "mainWindow must bind the title bar to the active product area");

        QVERIFY2(titleBar.contains(QStringLiteral("GTheme.titleBarHeight")), "TitleBar must use the central height token");
        QVERIFY2(titleBar.contains(QStringLiteral("AuroraBrand")), "TitleBar must use the Aurora brand asset");
        QVERIFY2(titleBar.contains(QStringLiteral("property string sectionTitle")), "TitleBar must expose sectionTitle");
        QVERIFY2(titleBar.contains(QStringLiteral("import QtQuick.Controls")),
                 "TitleBar must import QtQuick.Controls for ToolTip attached properties");

        QVERIFY2(navigator.contains(QStringLiteral("iconName: \"home\"")), "Navigator must use semantic Aurora SVG icons");
        QVERIFY2(navigator.contains(QStringLiteral("AuroraBrand")), "Navigator must use the Aurora brand asset");
        QVERIFY2(!navigator.contains(QStringLiteral("SegoeFluentIcons")), "Navigator must not use legacy Segoe Fluent glyphs");
        QVERIFY2(!navigator.contains(QStringLiteral("text: \"G\"")), "Navigator must not use the placeholder G logo");
    }

    void task_dialog_keeps_download_settings_visible_by_default() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/CommonComponents/TaskDialogPage.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        const qsizetype stackIndex = text.indexOf(QStringLiteral("objectName: \"taskDialogStack\""));
        QVERIFY2(stackIndex >= 0, "Task dialog StackLayout must expose objectName taskDialogStack");

        const qsizetype firstPageIndex = text.indexOf(QStringLiteral("// URL 输入页面"), stackIndex);
        QVERIFY2(firstPageIndex > stackIndex, "Task dialog StackLayout header should appear before URL page content");

        const QString stackHeader = text.mid(stackIndex, firstPageIndex - stackIndex);
        QVERIFY2(stackHeader.contains(QStringLiteral("Layout.preferredHeight:")),
                 "Task dialog StackLayout must use a bounded preferred height");
        QVERIFY2(!stackHeader.contains(QStringLiteral("Layout.fillHeight: true")),
                 "Task dialog StackLayout must not fill the scroll area before Download Settings");
    }

    void netdisk_page_exposes_workflow_hooks() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/CommonComponents/NetDiskPageView.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskPage\"")), "NetDiskPageView root must expose objectName netDiskPage");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskWorkflow\"")), "NetDisk workflow area must expose objectName netDiskWorkflow");
        QVERIFY2(text.contains(QStringLiteral("objectName: \"netDiskFileList\"")), "NetDisk file list must expose objectName netDiskFileList");
        QVERIFY2(text.contains(QStringLiteral("Paste link")), "NetDisk workflow must expose Paste link copy");
        QVERIFY2(text.contains(QStringLiteral("Preview files")), "NetDisk workflow must expose Preview files copy");
        QVERIFY2(text.contains(QStringLiteral("Add to queue")), "NetDisk workflow must expose Add to queue copy");
    }

    void download_flows_do_not_bind_layout_height_to_children_rect() {
        const QString path = QStringLiteral("%1/src/App/ui/Resource/qml/CommonComponents/GDownloadViewPage.qml").arg(QStringLiteral(SOURCE_ROOT));
        const QString text = readFile(path);
        QVERIFY2(!text.contains(QStringLiteral("Layout.preferredHeight: childrenRect.height")),
                 "Flow height must not feed childrenRect back into its parent QQuickLayout");
        QVERIFY2(!text.contains(QStringLiteral("implicitHeight: childrenRect.height")),
                 "Flow implicitHeight is read-only and must be provided by the positioner itself");
        QVERIFY2(!text.contains(QStringLiteral("implicitHeight: visible ? childrenRect.height : 0")),
                 "Hidden Flow items must collapse through visibility instead of assigning read-only implicitHeight");
    }

    void language_locale_defaults_and_legacy_values_are_canonicalized() {
        const QString languageManager = readFile(
            QStringLiteral("%1/src/App/ui/language/language_manager.cpp").arg(QStringLiteral(SOURCE_ROOT)));
        const QString setting = readFile(
            QStringLiteral("%1/src/App/ui/Settings/setting.h").arg(QStringLiteral(SOURCE_ROOT)));
        const QString configKeys = readFile(
            QStringLiteral("%1/src/Module/GDLCore/config/config_key.h").arg(QStringLiteral(SOURCE_ROOT)));

        QVERIFY2(languageManager.contains(QStringLiteral("locale.replace('-', '_')")),
                 "LanguageManager must accept legacy hyphenated locale values");
        QVERIFY2(languageManager.contains(QStringLiteral("if (lowered == \"zh_cn\") return \"zh_CN\";")),
                 "LanguageManager must map legacy zh-cn settings to the deployed zh_CN catalog");
        QVERIFY2(setting.contains(QStringLiteral("value_ = \"zh_CN\";")),
                 "Settings language default must use a supported canonical locale");
        QVERIFY2(configKeys.contains(QStringLiteral("CONFIG_PATH(Language, \"general.language\", \"zh_CN\")")),
                 "Config language default must match the deployed translation filename");
    }

    void application_shutdown_destroys_qml_before_managers() {
        const QString mainWindow = readFile(
            QStringLiteral("%1/src/App/ui/view/mainwindow.cxx").arg(QStringLiteral(SOURCE_ROOT)));
        const qsizetype rootCleanup = mainWindow.indexOf(QStringLiteral("delete root_object;"));
        const qsizetype managerCleanup = mainWindow.indexOf(QStringLiteral("UnInitEngine();"), rootCleanup);
        QVERIFY2(rootCleanup >= 0, "Application shutdown must explicitly destroy QML root objects");
        QVERIFY2(managerCleanup > rootCleanup,
                 "QML roots must be destroyed before managers are uninitialized");
        QVERIFY2(mainWindow.indexOf(QStringLiteral("engine.collectGarbage();"), rootCleanup) < managerCleanup,
                 "QML garbage collection must run before manager teardown");
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
