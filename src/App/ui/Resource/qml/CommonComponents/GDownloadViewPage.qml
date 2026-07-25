import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Aurora 下载生命周期列表。旧模型和 BrowserManager 行为保持不变，视觉按生命周期重组。
Control {
    id: downloadView

    property alias model: downloadListView.model
    property int pageType: -1 // 0 active, 1 waiting, 2 stopped

    readonly property int taskCount: downloadListView.count
    readonly property int failedTaskState: 4
    readonly property bool compactLayout: width < 680
    readonly property int pagePadding: compactLayout ? GTheme.spaceSM : GTheme.space2XL
    readonly property int taskIconSize: compactLayout ? GTheme.sizeDefault : GTheme.sizeLarge

    function refreshLayout() {
        downloadListView.forceLayout()
    }

    component MetaChip: Rectangle {
        id: metaChip

        property string label: ""
        property string value: ""
        property color accentColor: GTheme.textSecondary
        property color fillColor: GTheme.fillLighter
        property color borderColor: GTheme.borderLighter

        implicitWidth: Math.min(downloadView.width - downloadView.pagePadding * 2,
                                metaRow.implicitWidth + GTheme.spaceSM * 2)
        implicitHeight: GTheme.sizeSmall
        radius: GTheme.radiusRound
        color: fillColor
        border.width: 1
        border.color: borderColor

        Row {
            id: metaRow
            anchors.centerIn: parent
            spacing: GTheme.spaceXS

            Text {
                visible: metaChip.label.length > 0
                text: metaChip.label
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }

            Text {
                text: metaChip.value
                font.pixelSize: GTheme.fontCaption
                font.weight: GTheme.weightMedium
                color: metaChip.accentColor
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    component StatusBadge: Rectangle {
        id: statusBadge

        property string label: ""
        property string iconName: "info"
        property color accentColor: GTheme.infoColor
        property color fillColor: GTheme.bgInfo
        property color borderColor: GTheme.borderInfo

        implicitWidth: statusRow.implicitWidth + GTheme.spaceSM * 2
        implicitHeight: GTheme.sizeSmall
        radius: GTheme.radiusRound
        color: fillColor
        border.width: 1
        border.color: borderColor

        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: GTheme.spaceXS

            AuroraIcon {
                anchors.verticalCenter: parent.verticalCenter
                name: statusBadge.iconName
                iconSize: GTheme.fontCaption
                color: statusBadge.accentColor
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: statusBadge.label
                font.pixelSize: GTheme.fontCaption
                font.weight: GTheme.weightMedium
                color: statusBadge.accentColor
            }
        }
    }

    background: Rectangle {
        color: GTheme.bgPage

        EmptyState {
            objectName: "downloadEmptyState"
            anchors.centerIn: parent
            width: Math.max(0, Math.min(parent.width - GTheme.space2XL * 2, maximumContentWidth))
            opacity: downloadListView.count > 0 ? 0 : 1
            visible: opacity > 0
            iconName: downloadView.pageType === 1 ? "queue"
                                                    : (downloadView.pageType === 2 ? "completed" : "download")
            accentColor: downloadView.pageType === 1 ? GTheme.warningColor
                                                       : (downloadView.pageType === 2 ? GTheme.successColor
                                                                                     : GTheme.primaryColor)
            title: {
                switch (downloadView.pageType) {
                case 0: return qsTr("No active downloads")
                case 1: return qsTr("No waiting downloads")
                case 2: return qsTr("No stopped downloads")
                default: return qsTr("No downloads")
                }
            }
            description: downloadView.pageType === 2
                         ? qsTr("Completed and failed downloads will appear here.")
                         : qsTr("Add a download to begin building your queue.")

            Behavior on opacity {
                NumberAnimation {
                    duration: GTheme.durationBase
                    easing.type: GTheme.easingStandard
                }
            }

        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true

        ListView {
            id: downloadListView
            objectName: "downloadLifecycleList"
            spacing: GTheme.spaceSM
            topMargin: GTheme.spaceSM
            bottomMargin: GTheme.spaceLG
            leftMargin: downloadView.pagePadding
            rightMargin: downloadView.pagePadding
            clip: true
            interactive: true
            orientation: ListView.Vertical
            model: downloadView.model

            section.property: downloadView.pageType === 2 ? "taskState" : ""
            section.criteria: ViewSection.FullString
            section.delegate: Item {
                required property string section

                objectName: Number(section) === downloadView.failedTaskState
                            ? "failedSectionHeader" : "completedSectionHeader"
                width: downloadListView.width - downloadListView.leftMargin - downloadListView.rightMargin
                height: downloadView.pageType === 2 ? GTheme.sizeDefault + GTheme.spaceSM : 0
                visible: downloadView.pageType === 2
                clip: true

                RowLayout {
                    anchors.fill: parent
                    visible: downloadView.pageType === 2
                    spacing: GTheme.spaceSM

                    AuroraIcon {
                        name: Number(parent.parent.section) === downloadView.failedTaskState ? "error" : "completed"
                        iconSize: GTheme.fontBody
                        color: Number(parent.parent.section) === downloadView.failedTaskState
                               ? GTheme.dangerColor : GTheme.successColor
                    }

                    Text {
                        text: Number(parent.parent.section) === downloadView.failedTaskState
                              ? qsTr("Failed") : qsTr("Completed")
                        font.pixelSize: GTheme.fontSubtitle
                        font.weight: GTheme.weightDemiBold
                        color: Number(parent.parent.section) === downloadView.failedTaskState
                               ? GTheme.textDanger : GTheme.textSuccess
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Number(parent.parent.section) === downloadView.failedTaskState
                               ? GTheme.borderDanger : GTheme.borderSuccess
                    }
                }
            }

            delegate: GCard {
                id: taskCard

                objectName: "downloadTaskCard"
                readonly property bool activeTask: downloadView.pageType === 0 && model.taskState === 1
                readonly property bool pausedTask: downloadView.pageType === 0 && model.taskState !== 1
                readonly property bool waitingTask: downloadView.pageType === 1
                readonly property bool stoppedTask: downloadView.pageType === 2
                readonly property bool failedTask: stoppedTask && model.taskState === downloadView.failedTaskState
                readonly property bool completedTask: stoppedTask && !failedTask
                readonly property color stateColor: failedTask ? GTheme.dangerColor
                                                                : (completedTask ? GTheme.successColor
                                                                                 : (waitingTask ? GTheme.warningColor
                                                                                                : (pausedTask ? GTheme.infoColor
                                                                                                              : GTheme.primaryColor)))

                function openDeleteDialog() {
                    deleteConfirmDialog.pageType = downloadView.pageType
                    deleteConfirmDialog.taskFileName = model.fileName
                    deleteConfirmDialog.currentTaskId = model.taskId
                    deleteConfirmDialog.open()
                }

                width: downloadListView.width - downloadListView.leftMargin - downloadListView.rightMargin
                height: taskContent.implicitHeight + resolvedPadding * 2
                padding: downloadView.compactLayout ? GTheme.spaceSM : GTheme.spaceMD
                outlined: true
                hoverEnabled: true
                interactive: true
                radius: GTheme.radiusLarge
                variant: "elevated"
                selected: hovered

                    ColumnLayout {
                        id: taskContent
                        anchors.fill: parent
                        anchors.margins: taskCard.resolvedPadding
                        spacing: GTheme.spaceSM

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: GTheme.spaceSM

                        Rectangle {
                            Layout.preferredWidth: downloadView.taskIconSize
                            Layout.preferredHeight: downloadView.taskIconSize
                            Layout.minimumWidth: downloadView.taskIconSize
                            Layout.minimumHeight: downloadView.taskIconSize
                            Layout.alignment: Qt.AlignTop
                            radius: GTheme.radiusMedium
                            color: taskCard.failedTask ? GTheme.bgDanger
                                                       : (taskCard.completedTask ? GTheme.bgSuccess
                                                                                 : (taskCard.waitingTask ? GTheme.bgWarning
                                                                                                         : GTheme.primaryLight(9)))
                            border.width: 1
                            border.color: taskCard.failedTask ? GTheme.borderDanger
                                                              : (taskCard.completedTask ? GTheme.borderSuccess
                                                                                        : (taskCard.waitingTask ? GTheme.borderWarning
                                                                                                                : GTheme.primaryLight(7)))

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    const parts = String(model.fileName).split(".")
                                    if (parts.length <= 1) return qsTr("FILE")
                                    return parts[parts.length - 1].slice(0, 3).toUpperCase()
                                }
                                font.pixelSize: GTheme.fontCaption
                                font.weight: GTheme.weightDemiBold
                                color: taskCard.stateColor
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: 0
                            Layout.maximumWidth: Math.max(0,
                                taskCard.width - taskCard.leftPadding - taskCard.rightPadding
                                - downloadView.taskIconSize - 188 - (GTheme.spaceSM * 2))
                            spacing: GTheme.spaceXS

                            Text {
                                id: fileNameText
                                objectName: "taskFileName"
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: model.fileName
                                font.pixelSize: GTheme.fontBody
                                font.weight: GTheme.weightDemiBold
                                color: GTheme.textPrimary
                                wrapMode: downloadView.compactLayout ? Text.WrapAnywhere : Text.NoWrap
                                elide: Text.ElideRight
                                maximumLineCount: downloadView.compactLayout ? 2 : 1
                            }

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: {
                                    if (taskCard.waitingTask) {
                                        return qsTr("Ready to start when a download slot is available.")
                                    }
                                    if (taskCard.completedTask) {
                                        return qsTr("Saved to %1").arg(model.savePath)
                                    }
                                    if (taskCard.failedTask) {
                                        return qsTr("The transfer stopped before completion.")
                                    }
                                    return taskCard.activeTask
                                            ? qsTr("Downloading now") : qsTr("Paused — resume when ready")
                                }
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textSecondary
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            }
                        }

                        ColumnLayout {
                            id: taskTrailingColumn
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: 188
                            Layout.minimumWidth: 0
                            spacing: GTheme.spaceXS

                            StatusBadge {
                                Layout.alignment: Qt.AlignRight
                                label: taskCard.activeTask ? qsTr("Downloading")
                                                           : (taskCard.pausedTask ? qsTr("Paused")
                                                                                  : (taskCard.waitingTask ? qsTr("Queued")
                                                                                                          : (taskCard.failedTask ? qsTr("Failed")
                                                                                                                                 : qsTr("Completed"))))
                                iconName: taskCard.activeTask ? "download"
                                                             : (taskCard.pausedTask ? "pause"
                                                                                    : (taskCard.waitingTask ? "queue"
                                                                                                            : (taskCard.failedTask ? "error"
                                                                                                                                   : "completed")))
                                accentColor: taskCard.stateColor
                                fillColor: taskCard.failedTask ? GTheme.bgDanger
                                                               : (taskCard.completedTask ? GTheme.bgSuccess
                                                                                         : (taskCard.waitingTask ? GTheme.bgWarning
                                                                                                                 : GTheme.bgInfo))
                                borderColor: taskCard.failedTask ? GTheme.borderDanger
                                                                 : (taskCard.completedTask ? GTheme.borderSuccess
                                                                                           : (taskCard.waitingTask ? GTheme.borderWarning
                                                                                                                   : GTheme.borderInfo))
                            }

                            RowLayout {
                                id: actionFlow
                                objectName: "taskActionFlow"
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignRight
                                spacing: GTheme.spaceXS
                                layoutDirection: Qt.RightToLeft

                                GButton {
                                    objectName: taskCard.waitingTask ? "btnRemoveWaitingTask" : "btnDeleteTask"
                                    text: taskCard.waitingTask ? qsTr("Remove") : ""
                                    iconName: "delete"
                                    buttonType: "danger"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: taskCard.openDeleteDialog()
                                }

                                GButton {
                                    objectName: "btnCopyTaskLink"
                                    visible: String(model.downloadLink || "").trim().length > 0
                                    iconName: "link"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: {
                                        UtilsToolsManager.SetClipboardText(model.downloadLink)
                                        ToastManager.ShowSuccess(qsTr("Link copied to clipboard"))
                                    }
                                }

                                GButton {
                                    objectName: "btnOpenTaskLocation"
                                    visible: !taskCard.waitingTask && !taskCard.failedTask
                                    iconName: "folder"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: BrowserManager.OpenFileLocation(model.savePath)
                                }

                                GButton {
                                    objectName: "btnOpenCompletedTask"
                                    visible: taskCard.completedTask
                                    text: qsTr("Open")
                                    iconName: "open-file"
                                    buttonType: "success"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: Qt.openUrlExternally(model.savePath)
                                }

                                GButton {
                                    objectName: "btnRetryTask"
                                    visible: taskCard.failedTask && String(model.downloadLink || "").trim().length > 0
                                    text: qsTr("Retry")
                                    iconName: "refresh"
                                    buttonType: "primary"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: BrowserManager.RetryTask(model.taskId)
                                }

                                GButton {
                                    objectName: "btnStartWaitingTask"
                                    visible: taskCard.waitingTask
                                    text: qsTr("Start")
                                    iconName: "play"
                                    buttonType: "primary"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: BrowserManager.UnpauseTask(1, model.taskId)
                                }

                                GButton {
                                    objectName: "btnResumeTask"
                                    visible: taskCard.pausedTask
                                    text: qsTr("Resume")
                                    iconName: "play"
                                    buttonType: "primary"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: BrowserManager.UnpauseTask(0, model.taskId)
                                }

                                GButton {
                                    objectName: "btnPauseTask"
                                    visible: taskCard.activeTask
                                    text: qsTr("Pause")
                                    iconName: "pause"
                                    size: "small"
                                    width: implicitWidth
                                    height: implicitHeight
                                    onClicked: BrowserManager.PauseTask(0, model.taskId)
                                }
                            }
                        }
                    }

                    GProgressBar {
                        objectName: "taskTransferProgress"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 8
                        from: 0
                        to: 100
                        value: model.progress
                        fgColor: taskCard.activeTask ? GTheme.primaryColor : GTheme.infoColor
                        status: taskCard.activeTask ? "primary" : "info"
                        visible: downloadView.pageType === 0
                    }

                    Flow {
                        id: metadataFlow
                        objectName: "taskMetadataFlow"
                        visible: !taskCard.failedTask
                        Layout.fillWidth: true
                        spacing: GTheme.spaceXS

                        MetaChip {
                            objectName: "activeTransferredMetadata"
                            visible: downloadView.pageType === 0
                            label: qsTr("Transferred")
                            value: qsTr("%1 of %2").arg(model.currentSize).arg(model.totalSize)
                            accentColor: GTheme.textPrimary
                        }

                        MetaChip {
                            objectName: "activeSpeedMetadata"
                            visible: taskCard.activeTask
                            label: qsTr("Speed")
                            value: model.downloadSpeed
                            accentColor: GTheme.textSuccess
                            fillColor: GTheme.bgSuccess
                            borderColor: GTheme.borderSuccess
                        }

                        MetaChip {
                            objectName: "activeEtaMetadata"
                            visible: taskCard.activeTask
                            label: qsTr("ETA")
                            value: model.remainingTime
                            accentColor: GTheme.textPrimary
                        }

                        MetaChip {
                            visible: taskCard.activeTask
                            label: qsTr("Connections")
                            value: String(model.connections)
                            accentColor: GTheme.textPrimary
                        }

                        MetaChip {
                            objectName: "waitingQueuePosition"
                            visible: taskCard.waitingTask
                            label: qsTr("Queue")
                            value: qsTr("Position %1").arg(index + 1)
                            accentColor: GTheme.textWarning
                            fillColor: GTheme.bgWarning
                            borderColor: GTheme.borderWarning
                        }

                        MetaChip {
                            objectName: "waitingExpectedSize"
                            visible: taskCard.waitingTask
                            label: qsTr("Expected size")
                            value: model.totalSize
                            accentColor: GTheme.textPrimary
                        }

                        MetaChip {
                            objectName: "completedSizeMetadata"
                            visible: taskCard.completedTask
                            label: qsTr("Completed size")
                            value: model.totalSize
                            accentColor: GTheme.textSuccess
                            fillColor: GTheme.bgSuccess
                            borderColor: GTheme.borderSuccess
                        }

                        MetaChip {
                            visible: taskCard.completedTask
                            label: qsTr("Transferred")
                            value: model.currentSize
                            accentColor: GTheme.textPrimary
                        }
                    }

                    Text {
                        objectName: "taskErrorDetails"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        // 失败任务显示错误原因;未失败但带说明的任务(如 ed2k 等待可用源)也要显示——
                        // 否则用户面对的是一个毫无进展也没有任何解释的任务,不知道该继续等还是放弃。
                        visible: taskCard.failedTask || String(model.errorMessage || "").trim().length > 0
                        text: {
                            const code = String(model.errorCode || "").trim()
                            const message = String(model.errorMessage || "").trim()
                            if (!taskCard.failedTask) return message   // 状态说明,不加 "Error" 前缀
                            if (code.length > 0 && message.length > 0) {
                                return qsTr("Error %1: %2").arg(code).arg(message)
                            }
                            if (message.length > 0) return message
                            return qsTr("Download failed")
                        }
                        font.pixelSize: GTheme.fontCaption
                        // 未失败时是中性提示,用次要文本色;失败才用危险色,避免把"正在等待"渲染成故障
                        color: taskCard.failedTask ? GTheme.textDanger : GTheme.textSecondary
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }

                }
            }
        }
    }

    DeleteConfirmDialog {
        id: deleteConfirmDialog
        objectName: "deleteConfirmDialog"
        parent: Overlay.overlay

        property string currentTaskId: ""

        onActionSelected: function(action) {
            if (action === DeleteConfirmDialog.CancelAction) return

            const shouldDeleteFile = action === DeleteConfirmDialog.DeleteBothAction
            let result
            if (downloadView.pageType === 0) {
                result = BrowserManager.RemoveTask(0, currentTaskId, shouldDeleteFile)
            } else if (downloadView.pageType === 1) {
                result = BrowserManager.RemoveTask(1, currentTaskId, shouldDeleteFile)
            } else if (downloadView.pageType === 2) {
                result = BrowserManager.RemoveStopTask(currentTaskId, shouldDeleteFile)
            } else {
                return
            }

            const hasExpectedFields = result !== null
                    && typeof result === "object"
                    && typeof result.completeSuccess === "boolean"
                    && typeof result.partialSuccess === "boolean"
                    && !(result.completeSuccess && result.partialSuccess)
            if (!hasExpectedFields) {
                ToastManager.ShowError(qsTr("The task could not be removed."))
                return
            }

            if (result.completeSuccess === true) {
                ToastManager.ShowSuccess(shouldDeleteFile
                                         ? qsTr("Task and downloaded content were removed.")
                                         : qsTr("Task record was removed."))
            } else if (result.partialSuccess === true) {
                ToastManager.ShowWarning(shouldDeleteFile
                                         ? qsTr("The task was removed, but some downloaded content could not be deleted.")
                                         : qsTr("The task was removed, but cleanup could not be completed."))
            } else {
                ToastManager.ShowError(shouldDeleteFile
                                       ? qsTr("Failed to remove the task and downloaded content.")
                                       : qsTr("Failed to remove the task."))
            }
        }
    }
}
