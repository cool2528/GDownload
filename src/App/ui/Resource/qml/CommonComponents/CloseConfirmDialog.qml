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
    dialogWidth: 420
    standardHeight: 360

    // 自定义内容:提示文字 + "不再询问"选项
    customContent: Component {
        ColumnLayout {
            spacing: GTheme.spaceMD

            Label {
                text: qsTr("Tip: When minimized to tray, the application will continue running in the background. You can restore the window from the system tray.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            GCheckBox {
                id: dontAskAgainCheckbox
                text: qsTr("Don't ask again, remember my choice")
                checked: root.dontAskAgain
                onCheckedChanged: root.dontAskAgain = checked
            }
        }
    }

    // 按钮配置:取消 / 最小化到托盘(推荐) / 退出(危险)
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
            width: 90
        }
    ]
    defaultButtonIndex: -1

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
    }
}
