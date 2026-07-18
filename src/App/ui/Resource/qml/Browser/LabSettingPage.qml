import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./BrowserExtension"
import "../CommonComponents"
import gdl.sdk

// Aurora Lab page. The current experimental surface is the browser-extension
// workflow; behavior remains in the child cards while this page owns scrolling
// and responsive spacing.
Rectangle {
    id: labSetting
    objectName: "labSettingsPage"

    color: GTheme.bgPage
    clip: true

    readonly property bool compactLayout: width < 560
    readonly property int pagePadding: compactLayout ? GTheme.spaceMD : GTheme.spaceLG

    ScrollView {
        id: scrollView
        objectName: "labSettingsScrollView"
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        contentWidth: availableWidth
        contentHeight: pageColumn.implicitHeight + labSetting.pagePadding * 2

        ColumnLayout {
            id: pageColumn
            objectName: "labSettingsContent"
            x: labSetting.pagePadding
            y: labSetting.pagePadding
            width: Math.max(0, scrollView.availableWidth - labSetting.pagePadding * 2)
            spacing: GTheme.spaceMD

            GCard {
                id: headerCard
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                implicitHeight: headerLayout.implicitHeight + padding * 2
                outlined: true
                hoverEnabled: false
                interactive: false
                variant: "accentPrimary"
                padding: labSetting.compactLayout ? GTheme.spaceMD : GTheme.spaceLG
                radius: GTheme.radiusLarge

                background: Rectangle {
                    radius: headerCard.radius
                    color: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)
                    border.width: 1
                    border.color: GTheme.dark ? GTheme.borderBase : GTheme.primaryLight(7)
                }

                RowLayout {
                    id: headerLayout
                    anchors.fill: parent
                    anchors.margins: headerCard.padding
                    spacing: GTheme.spaceMD

                    Rectangle {
                        Layout.preferredWidth: GTheme.sizeLarge
                        Layout.preferredHeight: GTheme.sizeLarge
                        Layout.minimumWidth: GTheme.sizeLarge
                        Layout.minimumHeight: GTheme.sizeLarge
                        Layout.alignment: Qt.AlignTop
                        radius: GTheme.radiusMedium
                        color: GTheme.primaryColor

                        AuroraIcon {
                            anchors.centerIn: parent
                            name: "extension"
                            iconSize: GTheme.fontSubtitle
                            color: GTheme.textInverse
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: GTheme.spaceXS

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            text: qsTr("Browser Extension")
                            font.pixelSize: GTheme.fontTitle
                            font.weight: GTheme.weightDemiBold
                            color: GTheme.textPrimary
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            text: qsTr("Capture links from any webpage and send them directly to GDownload through a local connection.")
                            font.pixelSize: GTheme.fontBody
                            color: GTheme.textRegular
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }
                }
            }

            AlertTip {
                Layout.fillWidth: true
                severity: "warning"
                iconName: "lightbulb"
                title: qsTr("Experimental integration")
                description: qsTr("Review the connection values and browser permissions before enabling capture for authenticated websites.")
            }

            FeatureHighlightCard {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }

            InstallationGuideCard {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }

            ConfigHelperCard {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }

            FAQCard {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
        }
    }
}
