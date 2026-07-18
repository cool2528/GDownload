import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

/**
 * DeleteConfirmDialog - 删除确认对话框
 * 基于通用的 GMessageBox 组件实现
 */
GMessageBox {
    id: root
    objectName: "deleteConfirmDialog"

    // ========== 公开属性 ==========

    // 页面类型：0-正在下载 1-等待中 2-已完成
    property int pageType: -1

    // 任务文件名
    property string taskFileName: ""

    // 批量模式：复用同一套删除记录/删除文件的显式选择流程
    property bool batchMode: false

    // 内部属性：跟踪是否删除文件
    property bool deleteFileChecked: false

    // ========== 删除操作类型枚举 ==========

    enum DeleteAction {
        CancelAction,         // 取消
        DeleteTaskAction,     // 仅删除任务记录
        DeleteBothAction      // 删除任务和文件
    }

    // ========== 信号 ==========

    // 用户做出选择
    signal actionSelected(int action)

    // ========== 对话框配置 ==========

    title: qsTr("Delete Confirmation")
    messageType: GMessageBox.Warning
    dialogWidth: 460
    standardHeight: 330
    message: batchMode
             ? qsTr("Are you sure you want to delete all tasks in this list?")
             : qsTr("Are you sure you want to delete this download task?")

    // 自定义内容：文件名显示 + 删除选项
    customContent: Component {
        ColumnLayout {
            spacing: GTheme.spaceMD

            // 文件名显示框
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeLarge
                visible: !root.batchMode
                color: GTheme.fillLighter
                radius: GTheme.radiusLarge
                border.width: 1
                border.color: GTheme.borderBase

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: GTheme.spaceMD
                    anchors.rightMargin: GTheme.spaceMD
                    text: root.taskFileName
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textPrimary
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                    Accessible.role: Accessible.StaticText
                    Accessible.name: qsTr("Download task: %1").arg(text)
                }
            }

            // 删除文件选项（仅对正在下载和已完成的任务显示）
            GCheckBox {
                id: deleteFileCheckbox
                text: root.batchMode
                      ? qsTr("Also delete downloaded files")
                      : qsTr("Also delete downloaded file")
                checked: root.deleteFileChecked
                visible: root.pageType === 0 || root.pageType === 2
                Accessible.name: text

                onCheckedChanged: {
                    root.deleteFileChecked = checked
                }
            }

            // 每次打开重建复选框绑定:点击会打断 checked 声明式绑定,
            // 不重建会导致二次打开时视觉勾选与 deleteFileChecked 不一致(Q1)
            Connections {
                target: root
                function onOpened() {
                    deleteFileCheckbox.checked = Qt.binding(function() { return root.deleteFileChecked })
                }
            }

            Rectangle {
                Layout.fillWidth: true
                visible: root.pageType !== -1
                implicitHeight: deleteHintRow.implicitHeight + GTheme.spaceMD * 2
                radius: GTheme.radiusLarge
                color: root.deleteFileChecked && (root.pageType === 0 || root.pageType === 2)
                       ? GTheme.bgDanger : GTheme.bgInfo
                border.width: 1
                border.color: root.deleteFileChecked && (root.pageType === 0 || root.pageType === 2)
                              ? GTheme.borderDanger : GTheme.borderInfo

                RowLayout {
                    id: deleteHintRow
                    anchors.fill: parent
                    anchors.margins: GTheme.spaceMD
                    spacing: GTheme.spaceMD

                    AuroraIcon {
                        name: root.deleteFileChecked
                              && (root.pageType === 0 || root.pageType === 2)
                              ? "warning" : "info"
                        iconSize: GTheme.fontSubtitle
                        color: root.deleteFileChecked
                               && (root.pageType === 0 || root.pageType === 2)
                               ? GTheme.dangerColor : GTheme.infoColor
                        Layout.alignment: Qt.AlignTop
                    }

                    Label {
                        id: deleteHintText
                        text: {
                            if (root.pageType === 1) {
                                return root.batchMode
                                       ? qsTr("This will only remove all tasks from the waiting list.")
                                       : qsTr("This will only remove the task from the waiting list.")
                            } else if (root.deleteFileChecked
                                       && (root.pageType === 0 || root.pageType === 2)) {
                                return root.batchMode
                                       ? qsTr("Downloaded files will be permanently deleted and cannot be recovered.")
                                       : qsTr("The downloaded file will be permanently deleted and cannot be recovered.")
                            }
                            return root.batchMode
                                   ? qsTr("This will only remove task records. Downloaded files will be kept.")
                                   : qsTr("This will only remove the task record. The downloaded file will be kept.")
                        }
                        font.pixelSize: GTheme.fontCaption
                        color: root.deleteFileChecked
                               && (root.pageType === 0 || root.pageType === 2)
                               ? GTheme.dangerColor : GTheme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                    }
                }
            }
        }
    }

    // 按钮配置
    buttons: [
        {
            text: qsTr("Cancel"),
            type: "default",
            action: DeleteConfirmDialog.CancelAction
        },
        {
            text: qsTr("Delete"),
            type: "danger",
            action: DeleteConfirmDialog.DeleteTaskAction,
            width: 90
        }
    ]

    defaultButtonIndex: 0

    // ========== 事件处理 ==========

    onButtonClicked: function(index, buttonData) {
        if (index === 0) {
            // 取消按钮
            root.actionSelected(DeleteConfirmDialog.CancelAction)
        } else if (index === 1) {
            // 删除按钮 - 根据复选框状态决定操作类型
            if (root.deleteFileChecked) {
                root.actionSelected(DeleteConfirmDialog.DeleteBothAction)
            } else {
                root.actionSelected(DeleteConfirmDialog.DeleteTaskAction)
            }
        }
    }

    // 打开时重置状态
    onOpened: {
        root.deleteFileChecked = false
        Qt.callLater(root.focusDefaultButton)
    }
}
