import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Aurora 下载中心标题区：窄宽下操作区自动换到第二行，避免按钮挤出页面。
Rectangle {
    id: control
    property int type: 0
    signal addTaskRequested()
    readonly property bool compactLayout: width < 720
    readonly property bool narrowLayout: width < 560
    implicitHeight: compactLayout ? 116 : 88
    color: GTheme.bgPage

    // 底部分隔线
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: GTheme.borderLight
    }

    GridLayout {
        id: titleLayout
        anchors.fill: parent
        anchors.leftMargin: compactLayout ? GTheme.spaceSM : GTheme.space2XL
        anchors.rightMargin: compactLayout ? GTheme.spaceSM : GTheme.space2XL
        anchors.topMargin: GTheme.spaceSM
        anchors.bottomMargin: GTheme.spaceSM
        columns: compactLayout ? 1 : 2
        columnSpacing: GTheme.spaceLG
        rowSpacing: GTheme.spaceSM

        // 标题区域
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS

            Text {
                text: qsTr("Downloads")
                font.pixelSize: GTheme.fontH1
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
            }

            Text {
                text: {
                    switch(control.type) {
                        case 0: return qsTr("Everything is running smoothly · active downloads")
                        case 1: return qsTr("Queued tasks show position and expected size")
                        case 2: return qsTr("Review completed downloads and retry failures")
                        default: return qsTr("Manage your downloads")
                    }
                }
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: text.length > 0
            }
        }

        // 操作按钮区域。紧凑布局下独占第二行，主标题始终保留完整宽度。
        RowLayout {
            objectName: "downloadPageActions"
            Layout.fillWidth: compactLayout
            Layout.alignment: compactLayout ? Qt.AlignLeft : Qt.AlignRight
            spacing: GTheme.spaceSM

            GButton {
                objectName: "btnAddDownload"
                type: 1
                text: narrowLayout ? "" : qsTr("Add Download")
                iconName: "add"
                size: "default"
                onClicked: control.addTaskRequested()
            }

            GButton {
                objectName: "btnStartAllTasks"
                visible: control.type !== 2
                iconName: "play"
                onClicked: {
                    BrowserManager.UnpauseAllTask(control.type)
                }
            }

            GButton {
                objectName: "btnPauseAllTasks"
                visible: control.type !== 2
                iconName: "pause"
                onClicked: {
                    BrowserManager.PauseAllTask(control.type)
                }
            }

            Rectangle {
                visible: !compactLayout
                Layout.preferredWidth: 1
                Layout.preferredHeight: GTheme.spaceXL
                color: GTheme.borderLight
                Layout.alignment: Qt.AlignVCenter
            }

            GButton {
                objectName: "btnRefreshTasks"
                iconName: "refresh"
                onClicked: {
                    BrowserManager.RefreshTaskList(control.type)
                }
            }

            // 删除按钮
            GButton {
                objectName: "btnDeleteAllTasks"
                buttonType: "danger"
                iconName: "delete"
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
            if (action === DeleteConfirmDialog.CancelAction) {
                return
            }

            const shouldDeleteFiles = action === DeleteConfirmDialog.DeleteBothAction
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
