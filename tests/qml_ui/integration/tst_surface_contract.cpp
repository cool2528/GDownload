#include <QFile>
#include <QTest>

class TstSurfaceContract : public QObject {
    Q_OBJECT

private:
    static QByteArray readQml(const char* relativePath) {
        QFile file(QStringLiteral(SOURCE_ROOT) + QLatin1Char('/') + QString::fromLatin1(relativePath));
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

    static void verifyContains(const QByteArray& source, const char* contract) {
        QVERIFY2(source.contains(contract), contract);
    }

private slots:
    void cardUsesAuroraSurfacesAndFocus() {
        const auto source = readQml("src/App/ui/Resource/qml/CommonComponents/GCard.qml");
        QVERIFY(!source.isEmpty());
        verifyContains(source, "GTheme.surfaceBase");
        verifyContains(source, "GTheme.surfaceElevated");
        verifyContains(source, "GTheme.focusRing");
        verifyContains(source, "GTheme.elevation2");
    }

    void dialogUsesAuroraScrimSurfaceAndIcon() {
        const auto source = readQml("src/App/ui/Resource/qml/CommonComponents/GDialogShell.qml");
        QVERIFY(!source.isEmpty());
        verifyContains(source, "Overlay.modal");
        verifyContains(source, "GTheme.overlayScrim");
        verifyContains(source, "GTheme.surfaceElevated");
        verifyContains(source, "iconName: \"close\"");
        QVERIFY(!source.contains("SegoeFluentIcons.ChromeClose"));
    }

    void scrollAndProgressUseSemanticTokens() {
        const auto scroll = readQml("src/App/ui/Resource/qml/CommonComponents/CustomVScrollBar.qml");
        const auto progress = readQml("src/App/ui/Resource/qml/CommonComponents/GProgressBar.qml");
        QVERIFY(!scroll.isEmpty());
        QVERIFY(!progress.isEmpty());
        verifyContains(scroll, "GTheme.fillBase");
        verifyContains(scroll, "GTheme.focusRing");
        verifyContains(progress, "GTheme.successColor");
        verifyContains(progress, "GTheme.warningColor");
        verifyContains(progress, "GTheme.dangerColor");
        QVERIFY(!progress.contains("GradientStop"));
    }

    void dividerAndElevationUseThemeContracts() {
        const auto divider = readQml("src/App/ui/Resource/qml/CommonComponents/Divider.qml");
        const auto elevation = readQml("src/App/ui/Resource/qml/CommonComponents/GElevation.qml");
        QVERIFY(!divider.isEmpty());
        QVERIFY(!elevation.isEmpty());
        verifyContains(divider, "GTheme.borderLight");
        verifyContains(elevation, "root.elevation.color");
        QVERIFY(!elevation.contains("Qt.rgba"));
    }

    void taskSurfacesKeepContentInsideCardEdges() {
        const auto lifecycle = readQml("src/App/ui/Resource/qml/CommonComponents/GDownloadViewPage.qml");
        const auto taskDialog = readQml("src/App/ui/Resource/qml/CommonComponents/TaskDialogPage.qml");
        const auto netDisk = readQml("src/App/ui/Resource/qml/CommonComponents/NetDiskPageView.qml");
        QVERIFY(!lifecycle.isEmpty());
        QVERIFY(!taskDialog.isEmpty());
        QVERIFY(!netDisk.isEmpty());
        verifyContains(lifecycle, "anchors.margins: taskCard.resolvedPadding");
        verifyContains(taskDialog, "readonly property int cardPadding: GTheme.spaceMD");
        verifyContains(taskDialog, "anchors.margins: taskPage.cardPadding");
        verifyContains(netDisk, "anchors.margins: parserCard.padding");
        verifyContains(netDisk, "anchors.margins: GTheme.spaceMD");
    }
};

QTEST_MAIN(TstSurfaceContract)
#include "tst_surface_contract.moc"
