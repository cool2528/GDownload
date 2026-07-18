import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

/**
 * CloseConfirmDialog - Close confirmation dialog
 * 基于通用 GMessageBox 组件实现(消除重复的表头/背景/阴影)
 *
 * Features:
 * - Ask user whether to quit or minimize to tray
 * - "Don't ask again" option
 * - Integrated with GTheme system
 */
GMessageBox {
    id: root
    objectName: "closeConfirmDialog"

    // Center on the full-window overlay
    parent: Overlay.overlay

    // Close action enum
    enum CloseAction {
        Cancel,         // Cancel
        Quit,           // Quit application
        MinimizeToTray  // Minimize to system tray
    }

    // Signal: User made a selection
    signal actionSelected(int action, bool dontAskAgain)

    // 桥接复选框状态(customContent 在 Loader 内,通过 root 属性回传)
    property bool dontAskAgain: false

    // ========== 对话框配置 ==========

    title: qsTr("Close Confirmation")
    messageType: GMessageBox.Warning
    message: qsTr("Do you want to quit the application or minimize it to the system tray?")
    dialogWidth: 460
    standardHeight: 360

    // 自定义内容:提示文字 + "不再询问"选项
    customContent: Component {
        ColumnLayout {
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: trayTipRow.implicitHeight + GTheme.spaceMD * 2
                radius: GTheme.radiusLarge
                color: GTheme.bgInfo
                border.width: 1
                border.color: GTheme.borderInfo

                RowLayout {
                    id: trayTipRow
                    anchors.fill: parent
                    anchors.margins: GTheme.spaceMD
                    spacing: GTheme.spaceMD

                    AuroraIcon {
                        name: "info"
                        iconSize: GTheme.fontSubtitle
                        color: GTheme.infoColor
                        Layout.alignment: Qt.AlignTop
                    }

                    Label {
                        text: qsTr("When minimized to tray, GDownload keeps running in the background. Restore it from the system tray at any time.")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                    }
                }
            }

            GCheckBox {
                id: dontAskAgainCheckbox
                objectName: "chkDontAskAgain"
                text: qsTr("Remember my choice")
                checked: root.dontAskAgain
                Accessible.name: text
                onCheckedChanged: root.dontAskAgain = checked
            }

            // 每次打开重建复选框绑定:点击会打断 checked 绑定,避免二次打开视觉与 dontAskAgain 不一致(Q1)
            Connections {
                target: root
                function onOpened() {
                    dontAskAgainCheckbox.checked = Qt.binding(function() { return root.dontAskAgain })
                }
            }
        }
    }

    // 按钮配置:取消 / 最小化到托盘(推荐) / 退出(危险)
    // 退出按钮带 objectName "btnConfirmClose":确认关闭应用,供集成测试 findChild 定位
    buttons: [
        {
            text: qsTr("Cancel"),
            type: "default",
            width: 90
        },
        {
            text: qsTr("Minimize to Tray"),
            type: "primary",
            width: 130
        },
        {
            text: qsTr("Quit"),
            type: "danger",
            width: 90,
            objectName: "btnConfirmClose"
        }
    ]
    defaultButtonIndex: 1

    // ========== 事件处理 ==========

    onButtonClicked: function(index, buttonData) {
        if (index === 0) {
            root.actionSelected(CloseConfirmDialog.Cancel, false)
        } else if (index === 1) {
            root.actionSelected(CloseConfirmDialog.MinimizeToTray, root.dontAskAgain)
        } else if (index === 2) {
            root.actionSelected(CloseConfirmDialog.Quit, root.dontAskAgain)
        }
    }

    // 打开时重置状态
    onOpened: {
        root.dontAskAgain = false
        Qt.callLater(root.focusDefaultButton)
    }
}
