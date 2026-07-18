#include <QtTest>

#include <QFile>
#include <QStringList>

class TstAuroraIconContract : public QObject {
    Q_OBJECT

   private slots:
    void migrated_surfaces_do_not_reference_legacy_font_enums() {
        const QStringList files = {
            QStringLiteral("Browser/HomePage.qml"),
            QStringLiteral("Browser/SettingsPageView.qml"),
            QStringLiteral("CommonComponents/GButton.qml"),
            QStringLiteral("CommonComponents/GDialogShell.qml"),
            QStringLiteral("CommonComponents/QuickActionCard.qml"),
            QStringLiteral("CommonComponents/SummaryMetricCard.qml"),
            QStringLiteral("CommonComponents/TaskDialogHeader.qml"),
            QStringLiteral("CommonComponents/TaskDialogPage.qml"),
            QStringLiteral("CommonComponents/FontIcon.qml"),
        };

        for (const QString& relativePath : files) {
            const QString text = readQml(relativePath);
            QVERIFY2(!text.contains(QStringLiteral("SegoeFluentIcons")),
                     qPrintable(QStringLiteral("%1 must not depend on SegoeFluentIcons enum values").arg(relativePath)));
        }
    }

    void product_surfaces_use_aurora_icons_directly() {
        const QStringList files = {
            QStringLiteral("Browser/HomePage.qml"),
            QStringLiteral("Browser/SettingsPageView.qml"),
            QStringLiteral("CommonComponents/TaskDialogHeader.qml"),
            QStringLiteral("CommonComponents/TaskDialogPage.qml"),
        };

        for (const QString& relativePath : files) {
            const QString text = readQml(relativePath);
            QVERIFY2(text.contains(QStringLiteral("iconName")) || text.contains(QStringLiteral("AuroraIcon {")),
                     qPrintable(QStringLiteral("%1 must expose semantic Aurora icon usage").arg(relativePath)));
            QVERIFY2(!text.contains(QStringLiteral("FontIcon {")),
                     qPrintable(QStringLiteral("%1 must not instantiate FontIcon directly").arg(relativePath)));
        }
    }

    void reusable_controls_prefer_semantic_icons_and_keep_legacy_api() {
        const QStringList files = {
            QStringLiteral("CommonComponents/GButton.qml"),
            QStringLiteral("CommonComponents/GDialogShell.qml"),
            QStringLiteral("CommonComponents/QuickActionCard.qml"),
            QStringLiteral("CommonComponents/SummaryMetricCard.qml"),
            QStringLiteral("CommonComponents/FontIcon.qml"),
        };

        for (const QString& relativePath : files) {
            const QString text = readQml(relativePath);
            QVERIFY2(text.contains(QStringLiteral("property string iconName")),
                     qPrintable(QStringLiteral("%1 must expose semantic iconName").arg(relativePath)));
            QVERIFY2(text.contains(QStringLiteral("property int iconSource")),
                     qPrintable(QStringLiteral("%1 must preserve legacy iconSource API").arg(relativePath)));
            QVERIFY2(text.contains(QStringLiteral("AuroraIcon {")),
                     qPrintable(QStringLiteral("%1 must render semantic icons through AuroraIcon").arg(relativePath)));
        }

        const QString button = readQml(QStringLiteral("CommonComponents/GButton.qml"));
        QVERIFY2(button.contains(QStringLiteral("visible: control.hasSemanticIcon")),
                 "GButton semantic icon path must take priority in standard and nav variants");

        const QString shell = readQml(QStringLiteral("CommonComponents/GDialogShell.qml"));
        QVERIFY2(shell.contains(QStringLiteral("visible: shell.hasSemanticIcon")),
                 "GDialogShell semantic icon path must take priority over image and font fallbacks");

        const QString fontIcon = readQml(QStringLiteral("CommonComponents/FontIcon.qml"));
        QVERIFY2(fontIcon.contains(QStringLiteral("function legacySemanticName(value)")),
                 "FontIcon must adapt known numeric legacy values to semantic SVG names");
        QVERIFY2(!fontIcon.contains(QStringLiteral("Segoe Fluent Icons")),
                 "FontIcon must not load or render the legacy icon font");
    }

    void production_resources_do_not_load_the_legacy_icon_font() {
        const QString resource = readSource(QStringLiteral("src/App/ui/Resource/resource_icons.qrc"));
        const QString mainWindow = readSource(QStringLiteral("src/App/ui/view/mainwindow.cxx"));
        QVERIFY2(!resource.contains(QStringLiteral("SegoeFluentIcons.ttf")),
                 "The legacy icon font must not be packaged as a production resource");
        QVERIFY2(!mainWindow.contains(QStringLiteral("addApplicationFont")),
                 "Application startup must not load the legacy icon font");
        QVERIFY2(!mainWindow.contains(QStringLiteral("setContextProperty(\"FluentIcons\"")),
                 "Application startup must not publish the legacy font family context property");
    }

    void interactive_icon_controls_expose_keyboard_and_accessibility_hooks() {
        const QString button = readQml(QStringLiteral("CommonComponents/GButton.qml"));
        QVERIFY2(button.contains(QStringLiteral("activeFocusOnTab:")), "GButton must remain keyboard focusable");
        QVERIFY2(button.contains(QStringLiteral("Accessible.name:")), "GButton must expose an accessible name");

        const QString quickAction = readQml(QStringLiteral("CommonComponents/QuickActionCard.qml"));
        QVERIFY2(quickAction.contains(QStringLiteral("activeFocusOnTab:")), "QuickActionCard must be keyboard focusable");
        QVERIFY2(quickAction.contains(QStringLiteral("Accessible.name:")), "QuickActionCard must expose an accessible name");

        const QString header = readQml(QStringLiteral("CommonComponents/TaskDialogHeader.qml"));
        QVERIFY2(header.contains(QStringLiteral("Accessible.name: qsTr(\"Close dialog\")")),
                 "TaskDialogHeader close action must expose an accessible name");

        const QString shell = readQml(QStringLiteral("CommonComponents/GDialogShell.qml"));
        QVERIFY2(shell.contains(QStringLiteral("Accessible.name: qsTr(\"Close dialog\")")),
                 "GDialogShell close action must expose an accessible name");
    }

   private:
    QString readQml(const QString& relativePath) const {
        return readSource(QStringLiteral("src/App/ui/Resource/qml/%1").arg(relativePath));
    }

    QString readSource(const QString& relativePath) const {
        const QString path = QStringLiteral("%1/%2").arg(QStringLiteral(SOURCE_ROOT), relativePath);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTest::qFail(qPrintable(QStringLiteral("Failed to open %1").arg(path)), __FILE__, __LINE__);
            return QString();
        }
        return QString::fromUtf8(file.readAll());
    }
};

QTEST_MAIN(TstAuroraIconContract)
#include "tst_aurora_icon_contract.moc"
