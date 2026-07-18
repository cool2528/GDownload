import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// Aurora browser-extension capability overview. The grid collapses from
// three columns to two/one without changing the feature content.
GCard {
    id: featureCard
    objectName: "extensionFeatureCard"

    Layout.fillWidth: true
    implicitHeight: contentLayout.implicitHeight + padding * 2
    outlined: true
    hoverEnabled: false
    interactive: false
    variant: "elevated"
    padding: GTheme.spaceLG
    radius: GTheme.radiusLarge

    readonly property int gridColumns: width >= 760 ? 3 : (width >= 460 ? 2 : 1)

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: featureCard.padding
        spacing: GTheme.spaceLG

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Feature Highlights")
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Capture, filter, and hand off browser downloads without leaving your current page.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }

        GridLayout {
            id: featureGrid
            objectName: "extensionFeatureGrid"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            columns: featureCard.gridColumns
            columnSpacing: GTheme.spaceMD
            rowSpacing: GTheme.spaceMD

            FeatureItem {
                iconName: "link"
                title: qsTr("One-Click Capture")
                description: qsTr("Capture downloadable links from the page you are viewing.")
                accent: "primary"
            }

            FeatureItem {
                iconName: "lightning"
                title: qsTr("Batch Download")
                description: qsTr("Select multiple links and send the complete group at once.")
                accent: "warning"
            }

            FeatureItem {
                iconName: "palette"
                title: qsTr("Unified Experience")
                description: qsTr("Use the same Aurora visual language as the desktop application.")
                accent: "info"
            }

            FeatureItem {
                iconName: "lock"
                title: qsTr("Local Connection")
                description: qsTr("Connect directly to the local aria2c JSON-RPC endpoint.")
                accent: "success"
            }

            FeatureItem {
                iconName: "globe"
                title: qsTr("Cross-Browser")
                description: qsTr("Use the extension with Chrome, Firefox, and Edge.")
                accent: "primary"
            }

            FeatureItem {
                iconName: "filter"
                title: qsTr("Smart Filtering")
                description: qsTr("Filter links by size, type, domain, and custom rules.")
                accent: "info"
            }
        }
    }

    component FeatureItem: Rectangle {
        property string iconName: "info"
        property string title: ""
        property string description: ""
        property string accent: "primary"

        readonly property color accentColor: {
            switch (accent) {
            case "success": return GTheme.successColor
            case "warning": return GTheme.warningColor
            case "info": return GTheme.infoColor
            default: return GTheme.primaryColor
            }
        }
        readonly property color accentBackground: {
            switch (accent) {
            case "success": return GTheme.bgSuccess
            case "warning": return GTheme.bgWarning
            case "info": return GTheme.bgInfo
            default: return GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)
            }
        }

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: Math.max(GTheme.sizeLarge * 2,
                                         itemLayout.implicitHeight + GTheme.spaceMD * 2)
        radius: GTheme.radiusMedium
        color: hoverHandler.hovered ? GTheme.fillLight : GTheme.surfaceBase
        border.width: 1
        border.color: hoverHandler.hovered ? accentColor : GTheme.borderLighter

        Behavior on color {
            ColorAnimation { duration: GTheme.durationFast }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationFast }
        }

        HoverHandler {
            id: hoverHandler
        }

        RowLayout {
            id: itemLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.minimumWidth: GTheme.sizeLarge
                Layout.minimumHeight: GTheme.sizeLarge
                Layout.alignment: Qt.AlignTop
                radius: GTheme.radiusMedium
                color: accentBackground

                AuroraIcon {
                    anchors.centerIn: parent
                    name: iconName
                    iconSize: GTheme.fontSubtitle
                    color: accentColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.alignment: Qt.AlignTop
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: title
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: description
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
            }
        }
    }
}
