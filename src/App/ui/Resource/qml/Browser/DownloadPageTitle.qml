import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// V5 下载中心标题区:主标题 + 状态说明 + 添加任务 + 批量操作
Rectangle {
    id: control
    property int type: 0
    signal addTaskRequested()
    implicitHeight: GTheme.titleBarHeight + GTheme.spaceLG
    color: GTheme.bgPage  // 与下载中心背景一致

    // 底部分隔线
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: GTheme.borderLight
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: GTheme.space2XL  // 与左侧导航对齐
        anchors.rightMargin: GTheme.space2XL
        spacing: GTheme.spaceLG

        // 标题区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                text: qsTr("Downloads")
                font.pixelSize: GTheme.fontTitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
            }

            Text {
                text: {
                    switch(control.type) {
                        case 0: return qsTr("Everything is running smoothly · active downloads")
                        case 1: return qsTr("Queued tasks are ready to start")
                        case 2: return qsTr("Completed and stopped tasks")
                        default: return qsTr("Manage your downloads")
                    }
                }
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                visible: text.length > 0
            }
        }

        // 操作按钮区域 - 全部靠右
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: GTheme.spaceSM

            // 添加任务主按钮
            GButton {
                type: 1
                text: qsTr("Add Download")
                iconSource: SegoeFluentIcons.Add
                iconSize: GTheme.fontBody
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: control.addTaskRequested()
            }

            // 恢复/开始所有按钮
            GButton {
                visible: control.type !== 2
                iconSource: SegoeFluentIcons.PlayBadge12
                iconSize: GTheme.fontBody
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    BrowserManager.UnpauseAllTask(control.type)
                }
            }

            // 暂停所有按钮
            GButton {
                visible: control.type !== 2
                iconSource: SegoeFluentIcons.PauseBadge12
                iconSize: GTheme.fontBody
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    BrowserManager.PauseAllTask(control.type)
                }
            }

            // 分隔线
            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: GTheme.spaceXL
                color: GTheme.borderLight
                Layout.alignment: Qt.AlignVCenter
            }

            // 刷新按钮
            GButton {
                iconSource: SegoeFluentIcons.Refresh
                iconSize: GTheme.fontBody
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    BrowserManager.RefreshTaskList(control.type)
                }
            }

            // 删除按钮
            GButton {
                objectName: "btnDeleteAllTasks"
                buttonType: "danger"
                iconSource: SegoeFluentIcons.Delete
                iconSize: GTheme.fontBody
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    // 默认只删除任务记录；删除文件必须由用户显式勾选
                    deleteAllConfirmDialog.open()
                }
            }
        }
    }

    DeleteConfirmDialog {
        id: deleteAllConfirmDialog
        parent: Overlay.overlay
        batchMode: true
        pageType: control.type

        onActionSelected: function(action) {
            if (action === DeleteConfirmDialog.Cancel) {
                return
            }

            const shouldDeleteFiles = action === DeleteConfirmDialog.DeleteBoth
            const result = BrowserManager.RemoveAllTask(control.type, shouldDeleteFiles)
            const hasExpectedFields = result
                    && typeof result.total === "number"
                    && typeof result.complete === "number"
                    && typeof result.partial === "number"
                    && typeof result.failed === "number"
            const hasValidCounts = hasExpectedFields
                    && Number.isFinite(result.total)
                    && Number.isFinite(result.complete)
                    && Number.isFinite(result.partial)
                    && Number.isFinite(result.failed)
                    && Number.isInteger(result.total)
                    && Number.isInteger(result.complete)
                    && Number.isInteger(result.partial)
                    && Number.isInteger(result.failed)
                    && result.total > 0
                    && result.complete >= 0
                    && result.partial >= 0
                    && result.failed >= 0
                    && result.complete + result.partial + result.failed === result.total
            const total = hasValidCounts ? result.total : 0
            const complete = hasValidCounts ? result.complete : 0
            const partial = hasValidCounts ? result.partial : 0
            const failed = hasValidCounts ? result.failed : 0

            if (!hasValidCounts) {
                ToastManager.ShowError(qsTr("Failed to remove all tasks."))
            } else if (complete === total && partial === 0 && failed === 0) {
                ToastManager.ShowSuccess(shouldDeleteFiles
                                         ? qsTr("All tasks and downloaded content were removed.")
                                         : qsTr("All task records were removed."))
            } else if (failed > 0 && complete + partial > 0) {
                ToastManager.ShowWarning(qsTr("Some tasks were removed, but some tasks could not be removed."))
            } else if (partial > 0) {
                ToastManager.ShowWarning(shouldDeleteFiles
                                         ? qsTr("Some tasks were removed, but some downloaded content could not be deleted.")
                                         : qsTr("Some task records were removed, but some cleanup operations could not be completed."))
            } else {
                ToastManager.ShowError(qsTr("Failed to remove all tasks."))
            }
        }
    }
}
