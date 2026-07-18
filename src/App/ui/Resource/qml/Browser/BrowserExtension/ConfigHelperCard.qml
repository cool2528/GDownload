import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// Aurora connection handoff for the browser extension. Long endpoint and
// secret values stay selectable while action buttons move below them on a
// narrow page.
GCard {
    id: configCard
    objectName: "extensionConfigCard"

    Layout.fillWidth: true
    implicitHeight: contentLayout.implicitHeight + padding * 2
    outlined: true
    hoverEnabled: false
    interactive: false
    variant: "elevated"
    padding: GTheme.spaceLG
    radius: GTheme.radiusLarge

    readonly property bool compactLayout: width < 520
    readonly property string rpcUrl: "ws://127.0.0.1:" + SettingsManager.qRpcListenPort + "/jsonrpc"
    readonly property string rpcSecret: SettingsManager.qRpcSecret

    function copyToClipboard(value, successMessage) {
        UtilsToolsManager.SetClipboardText(value)
        ToastManager.ShowSuccess(successMessage, 2000)
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: configCard.padding
        spacing: GTheme.spaceLG

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.minimumWidth: GTheme.sizeLarge
                Layout.minimumHeight: GTheme.sizeLarge
                Layout.alignment: Qt.AlignTop
                radius: GTheme.radiusMedium
                color: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)

                AuroraIcon {
                    anchors.centerIn: parent
                    name: "settings"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Configuration Helper")
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Copy the local connection values into the browser extension options page.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }

        AlertTip {
            Layout.fillWidth: true
            severity: "success"
            iconName: "connected"
            title: qsTr("Local endpoint ready")
            description: qsTr("Keep GDownload running while the extension is connected.")
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD

            Text {
                Layout.fillWidth: true
                text: qsTr("Current GDownload Settings")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
            }

            ValueRow {
                label: qsTr("WebSocket URL")
                value: configCard.rpcUrl
                fieldObjectName: "extensionRpcUrlField"
                buttonObjectName: "extensionCopyUrlButton"
                onCopyRequested: configCard.copyToClipboard(configCard.rpcUrl,
                                                             qsTr("WebSocket URL copied."))
            }

            ValueRow {
                label: qsTr("RPC Secret")
                value: configCard.rpcSecret
                fieldObjectName: "extensionRpcSecretField"
                buttonObjectName: "extensionCopySecretButton"
                onCopyRequested: configCard.copyToClipboard(configCard.rpcSecret,
                                                             qsTr("RPC Secret copied."))
            }
        }

        Divider {
            Layout.fillWidth: true
            color: GTheme.borderLight
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceSM

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM

                AuroraIcon {
                    name: "lightbulb"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.warningColor
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Connect the extension")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                }
            }

            InstructionRow { text: qsTr("Copy the endpoint and secret shown above.") }
            InstructionRow { text: qsTr("Open the browser extension options page.") }
            InstructionRow { text: qsTr("Paste both values into the matching connection fields.") }
            InstructionRow { text: qsTr("Run Test Connection before saving the extension settings.") }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            columns: configCard.compactLayout ? 1 : 2
            columnSpacing: GTheme.spaceMD
            rowSpacing: GTheme.spaceSM

            GButton {
                objectName: "extensionCopyAllButton"
                Layout.fillWidth: configCard.compactLayout
                Layout.maximumWidth: configCard.compactLayout ? 100000 : 220
                text: qsTr("Copy All Settings")
                iconName: "copy"
                type: 1
                activeFocusOnTab: true
                Accessible.name: text
                onClicked: {
                    const allSettings = "WebSocket URL: " + configCard.rpcUrl + "\n" +
                                        "RPC Secret: " + configCard.rpcSecret
                    configCard.copyToClipboard(allSettings, qsTr("All connection settings copied."))
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.alignment: configCard.compactLayout ? Qt.AlignLeft : Qt.AlignVCenter
                text: qsTr("Need help? Open the FAQ below for connection troubleshooting.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                horizontalAlignment: configCard.compactLayout ? Text.AlignLeft : Text.AlignRight
            }
        }

        AlertTip {
            Layout.fillWidth: true
            severity: "warning"
            iconName: "warning"
            title: qsTr("GDownload must remain open")
            description: qsTr("The extension connects directly to the local aria2c service and cannot submit tasks after GDownload exits.")
        }
    }

    component ValueRow: ColumnLayout {
        property string label: ""
        property string value: ""
        property string fieldObjectName: ""
        property string buttonObjectName: ""
        signal copyRequested()

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: GTheme.spaceSM

        Text {
            Layout.fillWidth: true
            text: label
            font.pixelSize: GTheme.fontCaption
            font.weight: GTheme.weightMedium
            color: GTheme.textRegular
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            columns: configCard.compactLayout ? 1 : 2
            columnSpacing: GTheme.spaceSM
            rowSpacing: GTheme.spaceSM

            GTextField {
                objectName: fieldObjectName
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: value
                readOnly: true
                selectByMouse: true
                Accessible.name: label
            }

            GButton {
                objectName: buttonObjectName
                Layout.fillWidth: configCard.compactLayout
                Layout.preferredWidth: configCard.compactLayout ? implicitWidth : 112
                text: qsTr("Copy")
                iconName: "copy"
                activeFocusOnTab: true
                Accessible.name: qsTr("Copy %1").arg(label)
                onClicked: copyRequested()
            }
        }
    }

    component InstructionRow: RowLayout {
        property alias text: instructionText.text

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: GTheme.spaceSM

        AuroraIcon {
            Layout.alignment: Qt.AlignTop
            name: "completed"
            iconSize: GTheme.fontBody
            color: GTheme.successColor
        }

        Text {
            id: instructionText
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            font.pixelSize: GTheme.fontCaption
            color: GTheme.textRegular
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
    }
}
