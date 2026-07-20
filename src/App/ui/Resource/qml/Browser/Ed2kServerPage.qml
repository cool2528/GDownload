import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// eD2k 服务器页：列表(连接/移除) + 手动添加 + 从 URL 更新 + Kad 状态条
ColumnLayout {
    id: root
    spacing: GTheme.spaceMD

    property var serverModel: Ed2kManager.GetServerListModel()

    Component.onCompleted: {
        Ed2kManager.RefreshServers()
        Ed2kManager.RefreshKadStatus()
    }

    // 工具条
    RowLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceSM

        GButton {
            objectName: "ed2kServerUpdateFromUrl"
            text: qsTr("Update from URL")
            onClicked: updateUrlDialog.open()
        }
        GButton {
            objectName: "ed2kServerAdd"
            text: qsTr("Add Server")
            onClicked: addServerDialog.open()
        }
        Item { Layout.fillWidth: true }
        GButton {
            objectName: "ed2kServerDisconnect"
            visible: Ed2kManager.serverConnected
            text: qsTr("Disconnect")
            onClicked: Ed2kManager.DisconnectServer()
        }
    }

    // 服务器列表
    GCard {
        Layout.fillWidth: true
        Layout.fillHeight: true
        outlined: true
        padding: GTheme.spaceSM

        ListView {
            id: serverList
            objectName: "ed2kServerList"
            anchors.fill: parent
            clip: true
            spacing: GTheme.spaceXS
            model: root.serverModel
            ScrollBar.vertical: ScrollBar {}

            delegate: GCard {
                width: serverList.width
                outlined: true
                padding: GTheme.spaceSM

                RowLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceMD

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: model.connected ? GTheme.successColor : GTheme.textPlaceholder
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            text: model.serverName.length > 0 ? model.serverName : model.serverAddress
                            elide: Text.ElideRight
                            font.pixelSize: GTheme.fontBody
                            color: GTheme.textPrimary
                        }
                        Text {
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textSecondary
                            text: qsTr("%1 · %2 users · %3 files")
                                  .arg(model.serverAddress).arg(model.users).arg(model.files)
                        }
                    }
                    GButton {
                        objectName: "ed2kServerConnect" + index
                        text: model.connected ? qsTr("Connected") : qsTr("Connect")
                        enabled: !model.connected
                        onClicked: Ed2kManager.ConnectServer(model.serverIp, model.serverPort)
                    }
                    GButton {
                        objectName: "ed2kServerRemove" + index
                        iconName: "delete"
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Remove server")
                        onClicked: Ed2kManager.RemoveServer(model.serverIp, model.serverPort)
                    }
                }
            }
        }
    }

    // Kad 状态条
    GCard {
        Layout.fillWidth: true
        outlined: true
        padding: GTheme.spaceSM

        RowLayout {
            anchors.fill: parent
            spacing: GTheme.spaceSM
            Rectangle {
                width: 8; height: 8; radius: 4
                color: Ed2kManager.kadRunning ? GTheme.successColor : GTheme.textPlaceholder
            }
            Text {
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                text: Ed2kManager.kadRunning
                      ? qsTr("Kad: connected (%1 contacts)").arg(Ed2kManager.kadContacts)
                      : qsTr("Kad: not running")
            }
            Item { Layout.fillWidth: true }
        }
    }

    // 手动添加服务器对话框（复用项目统一对话框外壳 GDialogShell，非裸 Dialog）
    GDialogShell {
        id: addServerDialog
        objectName: "ed2kAddServerDialog"
        parent: Overlay.overlay
        width: Math.min(420, parent ? Math.max(0, parent.width - GTheme.spaceLG * 2) : 420)
        title: qsTr("Add Server")
        iconName: "add"
        initialFocusItem: addIpInput

        // 校验失败提示是否显示（点击 Add 后才出现，避免打开即报错）
        property bool showValidationError: false
        readonly property int parsedPort: parseInt(addPortInput.text, 10)
        readonly property bool ipValid: addIpInput.text.trim().length > 0
        readonly property bool portValid: parsedPort >= 1 && parsedPort <= 65535

        function reset() {
            addNameInput.text = ""
            addIpInput.text = ""
            addPortInput.text = "4661"
            showValidationError = false
        }

        onOpened: reset()

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: GTheme.spaceLG
            spacing: GTheme.spaceSM

            GTextField {
                id: addNameInput
                objectName: "ed2kAddServerName"
                Layout.fillWidth: true
                placeholderText: qsTr("Name (optional)")
            }
            GTextField {
                id: addIpInput
                objectName: "ed2kAddServerIp"
                Layout.fillWidth: true
                placeholderText: qsTr("IP address")
                status: addServerDialog.showValidationError && !addServerDialog.ipValid ? "danger" : "normal"
            }
            GTextField {
                id: addPortInput
                objectName: "ed2kAddServerPort"
                Layout.fillWidth: true
                placeholderText: qsTr("Port")
                text: "4661"
                status: addServerDialog.showValidationError && !addServerDialog.portValid ? "danger" : "normal"
            }
            Text {
                Layout.fillWidth: true
                visible: addServerDialog.showValidationError
                         && (!addServerDialog.ipValid || !addServerDialog.portValid)
                wrapMode: Text.WordWrap
                color: GTheme.dangerColor
                font.pixelSize: GTheme.fontCaption
                text: qsTr("Please enter an IP address and a port between 1 and 65535.")
            }
        }

        footer: Item {
            implicitHeight: addServerFooterRow.implicitHeight + GTheme.spaceMD * 2

            RowLayout {
                id: addServerFooterRow
                anchors.fill: parent
                anchors.leftMargin: GTheme.spaceLG
                anchors.rightMargin: GTheme.spaceLG
                anchors.topMargin: GTheme.spaceMD
                anchors.bottomMargin: GTheme.spaceMD
                spacing: GTheme.spaceSM

                Item { Layout.fillWidth: true }
                GButton {
                    objectName: "ed2kAddServerCancel"
                    text: qsTr("Cancel")
                    onClicked: addServerDialog.close()
                }
                GButton {
                    objectName: "ed2kAddServerConfirm"
                    type: 1
                    text: qsTr("Add")
                    onClicked: {
                        if (!addServerDialog.ipValid || !addServerDialog.portValid) {
                            addServerDialog.showValidationError = true
                            return
                        }
                        Ed2kManager.AddServer(addIpInput.text.trim(), addServerDialog.parsedPort,
                                               addNameInput.text.trim())
                        addServerDialog.close()
                    }
                }
            }
        }
    }

    // 从 URL 更新对话框
    GDialogShell {
        id: updateUrlDialog
        objectName: "ed2kUpdateUrlDialog"
        parent: Overlay.overlay
        width: Math.min(440, parent ? Math.max(0, parent.width - GTheme.spaceLG * 2) : 440)
        title: qsTr("Update Server List")
        iconName: "refresh"
        initialFocusItem: updateUrlInput

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: GTheme.spaceLG
            spacing: GTheme.spaceSM

            GTextField {
                id: updateUrlInput
                objectName: "ed2kUpdateUrlInput"
                Layout.fillWidth: true
                placeholderText: qsTr("server.met URL")
                text: qsTr("http://upd.emule-security.org/server.met")
            }
        }

        footer: Item {
            implicitHeight: updateUrlFooterRow.implicitHeight + GTheme.spaceMD * 2

            RowLayout {
                id: updateUrlFooterRow
                anchors.fill: parent
                anchors.leftMargin: GTheme.spaceLG
                anchors.rightMargin: GTheme.spaceLG
                anchors.topMargin: GTheme.spaceMD
                anchors.bottomMargin: GTheme.spaceMD
                spacing: GTheme.spaceSM

                Item { Layout.fillWidth: true }
                GButton {
                    objectName: "ed2kUpdateUrlCancel"
                    text: qsTr("Cancel")
                    onClicked: updateUrlDialog.close()
                }
                GButton {
                    objectName: "ed2kUpdateUrlConfirm"
                    type: 1
                    text: qsTr("Update")
                    enabled: updateUrlInput.text.trim().length > 0
                    onClicked: {
                        Ed2kManager.UpdateServersFromUrl(updateUrlInput.text.trim())
                        updateUrlDialog.close()
                    }
                }
            }
        }
    }
}
