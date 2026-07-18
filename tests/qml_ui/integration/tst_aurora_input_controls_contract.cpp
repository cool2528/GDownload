#include <QtTest>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QStringList>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"

using namespace gdl::tests;

class TstAuroraInputControlsContract : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase() {
        qputenv("GDOWNLOAD_TEST", "1");
        fakeBrowser_ = new FakeBrowserManager(this);
        fakeSettings_ = new FakeSettingsManager(this);
        setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
    }

    void fields_expose_semantic_states() {
        const QStringList fields = {
            QStringLiteral("GTextField.qml"),
            QStringLiteral("GComBoBox.qml"),
            QStringLiteral("GSpinBox.qml"),
        };

        for (const QString& fileName : fields) {
            const QString text = readComponent(fileName);
            QVERIFY2(text.contains(QStringLiteral("property string status")),
                     qPrintable(fileName + QStringLiteral(" must expose a semantic validation status")));
            QVERIFY2(text.contains(QStringLiteral("GTheme.focusRing")),
                     qPrintable(fileName + QStringLiteral(" must use the Aurora focus-ring token")));
            QVERIFY2(text.contains(QStringLiteral("border.width: 2")),
                     qPrintable(fileName + QStringLiteral(" must render a two-pixel outside focus ring")));
            QVERIFY2(text.contains(QStringLiteral("GTheme.borderDanger")),
                     qPrintable(fileName + QStringLiteral(" must expose a danger/error outline")));
            QVERIFY2(text.contains(QStringLiteral("GTheme.sizeLarge")),
                     qPrintable(fileName + QStringLiteral(" must preserve a 40 px default hit target")));
        }

        QVERIFY(readComponent(QStringLiteral("GTextField.qml")).contains(QStringLiteral("control.readOnly")));
        QVERIFY(readComponent(QStringLiteral("GComBoBox.qml")).contains(QStringLiteral("property bool readOnly")));
        QVERIFY(readComponent(QStringLiteral("GSpinBox.qml")).contains(QStringLiteral("!control.editable")));
    }

    void selection_controls_keep_value_and_interaction_states_separate() {
        const QString checkbox = readComponent(QStringLiteral("GCheckBox.qml"));
        const QString buttonSwitch = readComponent(QStringLiteral("GButtonSwitch.qml"));

        for (const QString& text : {checkbox, buttonSwitch}) {
            QVERIFY(text.contains(QStringLiteral("control.checked")));
            QVERIFY(text.contains(QStringLiteral("control.hovered")));
            QVERIFY(text.contains(QStringLiteral("control.down")));
            QVERIFY(text.contains(QStringLiteral("control.activeFocus")));
            QVERIFY(text.contains(QStringLiteral("GTheme.focusRing")));
            QVERIFY(text.contains(QStringLiteral("implicitHeight: GTheme.sizeLarge")));
        }
    }

    void image_button_uses_accessible_hit_and_focus_geometry() {
        const QString text = readComponent(QStringLiteral("GImageButton.qml"));
        QVERIFY(text.contains(QStringLiteral("implicitWidth: GTheme.sizeLarge")));
        QVERIFY(text.contains(QStringLiteral("implicitHeight: GTheme.sizeLarge")));
        QVERIFY(text.contains(QStringLiteral("GTheme.focusRing")));
        QVERIFY(text.contains(QStringLiteral("control.hovered && control.hasHoverImage")));
        QVERIFY(text.contains(QStringLiteral("control.down ? 0.9 : 1.0")));
    }

    void task_option_cards_use_accessible_names_and_minimum_hit_targets() {
        const QString advanced = readSource(
            QStringLiteral("src/App/ui/Resource/qml/CommonComponents/TaskAdvancedOptionsCard.qml"));
        const QString general = readSource(
            QStringLiteral("src/App/ui/Resource/qml/CommonComponents/TaskGeneralOptionsCard.qml"));

        QCOMPARE(advanced.count(QStringLiteral("Layout.preferredHeight: GTheme.sizeLarge")), 4);
        QVERIFY(advanced.contains(QStringLiteral("Accessible.name: qsTr(\"User-Agent\")")));
        QVERIFY(advanced.contains(QStringLiteral("Accessible.name: qsTr(\"Authorization header\")")));
        QVERIFY(advanced.contains(QStringLiteral("Accessible.name: qsTr(\"Referrer URL\")")));
        QVERIFY(advanced.contains(QStringLiteral("Accessible.name: qsTr(\"Cookie header\")")));
        QVERIFY(advanced.contains(QStringLiteral("Accessible.name: qsTr(\"Custom request headers\")")));
        QVERIFY(advanced.contains(QStringLiteral("activeFocusOnTab: enabled && visible")));
        for (const QString& torrentControl :
             {QStringLiteral("taskTorrentTrackerUrls"), QStringLiteral("taskTorrentDhtSwitch"),
              QStringLiteral("taskTorrentPeerLimit"), QStringLiteral("taskTorrentEncryption"),
              QStringLiteral("taskTorrentSeedRatio"), QStringLiteral("taskTorrentSeedTime")}) {
            QVERIFY2(advanced.contains(QStringLiteral("objectName: \"%1\"").arg(torrentControl)),
                     qPrintable(QStringLiteral("Missing torrent task option: %1").arg(torrentControl)));
        }
        for (const QString& aria2Option :
             {QStringLiteral("bt-tracker"), QStringLiteral("enable-dht"),
              QStringLiteral("bt-max-peers"), QStringLiteral("bt-require-crypto"),
              QStringLiteral("bt-force-encryption"), QStringLiteral("seed-ratio"),
              QStringLiteral("seed-time")}) {
            QVERIFY2(advanced.contains(QStringLiteral("options[\"%1\"]").arg(aria2Option)),
                     qPrintable(QStringLiteral("Missing aria2 torrent option mapping: %1")
                                    .arg(aria2Option)));
        }

        QCOMPARE(general.count(QStringLiteral("Layout.preferredHeight: GTheme.sizeLarge")), 3);
        QVERIFY(general.contains(QStringLiteral("Accessible.name: qsTr(\"Filename\")")));
        QVERIFY(general.contains(QStringLiteral("Accessible.name: qsTr(\"Connection splits\")")));
    }

    void title_bar_window_actions_have_names_and_bounded_hit_targets() {
        const QString titleBar =
            readSource(QStringLiteral("src/App/ui/Resource/qml/titlebar/TitleBar.qml"));

        for (const QString& accessibleName :
             {QStringLiteral("Close window"), QStringLiteral("Minimize window"),
              QStringLiteral("Enter full screen"), QStringLiteral("Minimize"),
              QStringLiteral("Maximize"), QStringLiteral("Restore"), QStringLiteral("Close")}) {
            QVERIFY2(titleBar.contains(
                         QStringLiteral("Accessible.name: qsTr(\"%1\")").arg(accessibleName)),
                     qPrintable(QStringLiteral("Missing accessible title-bar action: %1")
                                    .arg(accessibleName)));
        }

        QCOMPARE(titleBar.count(QStringLiteral("Layout.preferredHeight: 15")), 0);
        QCOMPARE(titleBar.count(QStringLiteral("Layout.preferredWidth: 15")), 0);
        QVERIFY(titleBar.count(QStringLiteral("Layout.preferredHeight: GTheme.titleBarHeight")) >= 7);
        QVERIFY(titleBar.count(QStringLiteral("Layout.preferredWidth: GTheme.sizeDefault")) >= 3);
    }

    void repository_link_is_not_pointer_only() {
        const QString navigator =
            readSource(QStringLiteral("src/App/ui/Resource/qml/Navigator/NavigatorView.qml"));

        QVERIFY(navigator.contains(QStringLiteral("objectName: \"repositoryLink\"")));
        QVERIFY(navigator.contains(QStringLiteral("activeFocusOnTab: visible")));
        QVERIFY(navigator.contains(QStringLiteral("Accessible.role: Accessible.Link")));
        QVERIFY(navigator.contains(
            QStringLiteral("Accessible.name: qsTr(\"Open GDownload repository\")")));
        QVERIFY(navigator.contains(QStringLiteral("Keys.onReturnPressed")));
        QVERIFY(navigator.contains(QStringLiteral("Keys.onEnterPressed")));
        QVERIFY(navigator.contains(QStringLiteral("Keys.onSpacePressed")));
        QVERIFY(navigator.contains(QStringLiteral("GTheme.focusRing")));
    }

    void settings_fields_expose_contextual_accessible_names() {
        const QList<QPair<QString, QStringList>> expectations = {
            {QStringLiteral("Browser/Aria2RpcSettingPage.qml"),
             {QStringLiteral("RPC listen port"), QStringLiteral("RPC secret"),
              QStringLiteral("Show RPC secret")}},
            {QStringLiteral("Browser/BitTorrentAdvancedSettingPage.qml"),
             {QStringLiteral("Enable DHT"), QStringLiteral("Maximum peers per torrent"),
              QStringLiteral("Require encrypted connections")}},
            {QStringLiteral("Browser/BasicSettingPage.qml"),
             {QStringLiteral("Display language"), QStringLiteral("Global proxy address")}},
            {QStringLiteral("Browser/ConnectionPerformanceSettingPage.qml"),
             {QStringLiteral("Maximum concurrent downloads"),
              QStringLiteral("Maximum connections per server"),
              QStringLiteral("Download splits"), QStringLiteral("Minimum split size")}},
            {QStringLiteral("Browser/SpeedControlSettingPage.qml"),
             {QStringLiteral("Global download speed limit"),
              QStringLiteral("Global upload speed limit"),
              QStringLiteral("Lowest download speed limit")}},
            {QStringLiteral("Browser/TimeoutRetrySettingPage.qml"),
             {QStringLiteral("Connection timeout"),
              QStringLiteral("Initial connection timeout"),
              QStringLiteral("Maximum retry attempts"), QStringLiteral("Retry wait time")}},
            {QStringLiteral("Browser/TrackerServerSettingPage.qml"),
             {QStringLiteral("Automatically update tracker sources"),
              QStringLiteral("Tracker server update result")}},
            {QStringLiteral("Browser/UserAgentSettingPage.qml"),
             {QStringLiteral("User-Agent preset"), QStringLiteral("Custom User-Agent")}},
            {QStringLiteral("Browser/PostDownloadActionsSettingPage.qml"),
             {QStringLiteral("Action when download completes"),
              QStringLiteral("Command to run when download completes"),
              QStringLiteral("Action when download fails"),
              QStringLiteral("Command to run when download fails"),
              QStringLiteral("Action when download starts")}},
        };

        for (const auto& expectation : expectations) {
            const QString source = readSource(
                QStringLiteral("src/App/ui/Resource/qml/%1").arg(expectation.first));
            for (const QString& name : expectation.second) {
                QVERIFY2(source.contains(
                             QStringLiteral("Accessible.name: qsTr(\"%1\")").arg(name)),
                         qPrintable(QStringLiteral("%1 must expose accessible name '%2'")
                                        .arg(expectation.first, name)));
            }
        }
    }

    void language_selector_uses_english_translation_sources() {
        const QString basic =
            readSource(QStringLiteral("src/App/ui/Resource/qml/Browser/BasicSettingPage.qml"));
        const QStringList languageSources = {
            QStringLiteral("English"),
            QStringLiteral("Simplified Chinese"),
            QStringLiteral("Traditional Chinese"),
            QStringLiteral("Japanese"),
            QStringLiteral("Korean"),
        };

        for (const QString& language : languageSources) {
            QVERIFY2(basic.contains(QStringLiteral("qsTr(\"%1\")").arg(language)),
                     qPrintable(QStringLiteral("Language display source must be English and translated: %1")
                                    .arg(language)));
        }

        for (const QString& forbiddenLiteral :
             {QStringLiteral("\"简体中文\""), QStringLiteral("\"繁體中文\""),
              QStringLiteral("\"日本語\""), QStringLiteral("\"한국어\"")}) {
            QVERIFY2(!basic.contains(forbiddenLiteral),
                     qPrintable(QStringLiteral("BasicSettingPage must not embed %1 as source text")
                                    .arg(forbiddenLiteral)));
        }
    }

    void core_inputs_follow_forward_and_reverse_tab_order() {
        QQmlComponent component(&engine_);
        component.setData(R"QML(
            import QtQuick
            import QtQuick.Layouts
            import gdl.sdk 1.0
            import "qrc:/qml/CommonComponents" as Components

            Item {
                width: 640
                height: 96

                RowLayout {
                    anchors.fill: parent

                    Components.GTextField {
                        objectName: "firstField"
                        Accessible.name: qsTr("First field")
                    }
                    Components.GComBoBox {
                        objectName: "secondField"
                        model: ["One", "Two"]
                        Accessible.name: qsTr("Second field")
                    }
                    Components.GSpinBox {
                        objectName: "thirdField"
                        Accessible.name: qsTr("Third field")
                    }
                }
            }
        )QML",
                          QUrl(QStringLiteral("qrc:/tests/qml_ui/support/KeyboardOrder.qml")));
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(!root.isNull(), qPrintable(component.errorString()));

        QQuickWindow window;
        window.resize(640, 96);
        auto* rootItem = qobject_cast<QQuickItem*>(root.data());
        QVERIFY(rootItem);
        rootItem->setParentItem(window.contentItem());
        window.show();

        auto* first = root->findChild<QQuickItem*>(QStringLiteral("firstField"));
        QVERIFY(first);
        QVERIFY(QMetaObject::invokeMethod(first, "forceActiveFocus"));
        QTRY_COMPARE(focusOwnerName(window.activeFocusItem()), QStringLiteral("firstField"));

        QTest::keyClick(&window, Qt::Key_Tab);
        QTRY_COMPARE(focusOwnerName(window.activeFocusItem()), QStringLiteral("secondField"));

        QTest::keyClick(&window, Qt::Key_Tab);
        QTRY_COMPARE(focusOwnerName(window.activeFocusItem()), QStringLiteral("thirdField"));

        QTest::keyClick(&window, Qt::Key_Tab, Qt::ShiftModifier);
        QTRY_COMPARE(focusOwnerName(window.activeFocusItem()), QStringLiteral("secondField"));
    }

    void user_facing_literal_bindings_are_translation_ready() {
        const QString qmlRoot =
            QStringLiteral("%1/src/App/ui/Resource/qml").arg(QStringLiteral(SOURCE_ROOT));
        const QRegularExpression rawBinding(
            QStringLiteral(R"(\b(?:text|title|subtitle|description|label|hint|placeholderText|message|question|answer|sectionTitle|statusText|defaultStatusText|actionText|tipText)\s*:\s*\"([^\"]*)\")"));
        const QRegularExpression rawStatusAssignment(
            QStringLiteral(R"(\bstatusText\s*=\s*\"([^\"]*)\")"));
        const QRegularExpression rawNotification(
            QStringLiteral(R"(\bToastManager\.Show(?:Success|Error|Info|Warning)\s*\(\s*\"([^\"]+)\")"));
        QStringList offenders;

        QDirIterator it(qmlRoot, QStringList() << QStringLiteral("*.qml"), QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFile file(path);
            QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(path));

            int lineNumber = 0;
            while (!file.atEnd()) {
                const QString line = QString::fromUtf8(file.readLine());
                ++lineNumber;
                const QString trimmed = line.trimmed();
                if (trimmed.startsWith(QStringLiteral("//")) ||
                    trimmed.startsWith(QLatin1Char('*'))) {
                    continue;
                }

                for (const QRegularExpression& expression :
                     {rawBinding, rawStatusAssignment, rawNotification}) {
                    const QRegularExpressionMatch match = expression.match(line);
                    if (!match.hasMatch()) {
                        continue;
                    }
                    const QString value = match.captured(1);
                    if (isNonTranslatableLiteral(value)) {
                        continue;
                    }
                    offenders << QStringLiteral("%1:%2 -> %3")
                                     .arg(QDir(qmlRoot).relativeFilePath(path))
                                     .arg(lineNumber)
                                     .arg(value);
                }
            }
        }

        QVERIFY2(offenders.isEmpty(),
                 qPrintable(QStringLiteral("User-facing literal bindings must use qsTr():\n%1")
                                .arg(offenders.join(QLatin1Char('\n')))));
    }

    void translation_sources_use_english_text_instead_of_status_glyphs() {
        const QString qmlRoot =
            QStringLiteral("%1/src/App/ui/Resource/qml").arg(QStringLiteral(SOURCE_ROOT));
        const QRegularExpression translationSource(
            QStringLiteral(R"(qsTr\s*\(\s*\"((?:\\.|[^\"\\])*)\")"));
        const QRegularExpression forbiddenCharacters(
            QStringLiteral("[\\x{3400}-\\x{9FFF}\\x{2713}\\x{2717}\\x{26A0}]"));
        QStringList offenders;

        QDirIterator it(qmlRoot, QStringList() << QStringLiteral("*.qml"), QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            const QString source = readSource(QDir(QStringLiteral(SOURCE_ROOT)).relativeFilePath(path));
            QRegularExpressionMatchIterator matches = translationSource.globalMatch(source);
            while (matches.hasNext()) {
                const QString value = matches.next().captured(1);
                if (forbiddenCharacters.match(value).hasMatch()) {
                    offenders << QStringLiteral("%1 -> %2")
                                     .arg(QDir(qmlRoot).relativeFilePath(path), value);
                }
            }
        }

        QVERIFY2(offenders.isEmpty(),
                 qPrintable(QStringLiteral("qsTr() sources must use English status text, not Han/status glyphs:\n%1")
                                .arg(offenders.join(QLatin1Char('\n')))));
    }

    void task_and_cloud_workflows_expose_keyboard_and_accessible_paths() {
        const QString taskDialog = readComponent(QStringLiteral("TaskDialogPage.qml"));
        QVERIFY(taskDialog.contains(QStringLiteral("Accessible.name: qsTr(\"Download URLs\")")));
        QVERIFY(taskDialog.contains(QStringLiteral("objectName: \"btnCancelTask\"")));
        QVERIFY(taskDialog.contains(QStringLiteral("objectName: \"taskDialogValidationAlert\"")));

        const QString netDisk = readComponent(QStringLiteral("NetDiskPageView.qml"));
        QVERIFY(netDisk.contains(QStringLiteral("Accessible.name: qsTr(\"Baidu share link\")")));
        QVERIFY(netDisk.contains(QStringLiteral("Accessible.name: qsTr(\"Select %1\").arg(model.fileName)")));
        QVERIFY(netDisk.contains(QStringLiteral("Keys.onReturnPressed")));
        QVERIFY(netDisk.contains(QStringLiteral("Keys.onEnterPressed")));
        QVERIFY(netDisk.contains(QStringLiteral("Keys.onSpacePressed")));
    }

   private:
    static QString focusOwnerName(QQuickItem* item) {
        QQuickItem* current = item;
        while (current) {
            if (!current->objectName().isEmpty()) {
                return current->objectName();
            }
            current = current->parentItem();
        }
        return {};
    }

    static bool isNonTranslatableLiteral(const QString& value) {
        if (value.isEmpty() || value == QStringLiteral("GDownload") ||
            value == QStringLiteral("[http://][USER:PASSWORD@]HOST[:PORT]")) {
            return true;
        }

        static const QRegularExpression numericLiteral(QStringLiteral(R"(^[0-9]+$)"));
        return numericLiteral.match(value).hasMatch();
    }

    static QString readSource(const QString& relativePath) {
        const QString normalized = QDir::fromNativeSeparators(relativePath);
        const QString path = QStringLiteral("%1/%2").arg(QStringLiteral(SOURCE_ROOT), normalized);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTest::qFail(qPrintable(QStringLiteral("Unable to read %1").arg(path)), __FILE__, __LINE__);
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }

    static QString readComponent(const QString& fileName) {
        return readSource(
            QStringLiteral("src/App/ui/Resource/qml/CommonComponents/%1").arg(fileName));
    }

    QQmlEngine engine_;
    FakeBrowserManager* fakeBrowser_ = nullptr;
    FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstAuroraInputControlsContract)
#include "tst_aurora_input_controls_contract.moc"
