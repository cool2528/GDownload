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
                buttonType: "danger"
                iconSource: SegoeFluentIcons.Delete
                iconSize: GTheme.fontBody
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    // 删除全部任务不可逆（含文件），需确认后执行
                    deleteAllConfirmDialog.open()
                }
            }

            // 删除全部任务确认对话框
            Dialog {
                id: deleteAllConfirmDialog
                parent: Overlay.overlay  // 以全窗口遮罩为定位父级，避免以按钮簇为父导致弹窗错位(X1)
                anchors.centerIn: parent
                modal: true
                width: 420
                title: qsTr("Delete Confirmation")
                standardButtons: Dialog.Cancel | Dialog.Ok

                contentItem: Text {
                    text: qsTr("Are you sure you want to delete all tasks in this list? This action cannot be undone.")
                    wrapMode: Text.WordWrap
                    color: GTheme.textPrimary
                }

                onAccepted: {
                    BrowserManager.RemoveAllTask(control.type, true)
                }
            }
        }
    }
}
