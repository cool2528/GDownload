import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Element Plus 风格下载列表页面
Control {
    id: downloadView
    property alias model: downloadListView.model
    property int pageType: -1 // 0 downloadPage 1 waitingPage  2 completedPage
    readonly property int taskCount: downloadListView.count
    readonly property int taskCardHeight: 104
    readonly property int failedTaskCardHeight: 120
    readonly property int taskIconSize: GTheme.sizeLarge
    readonly property int failedTaskState: 4

    background: Rectangle {
        color: GTheme.bgPage

        // 空状态显示
        Item {
            objectName: "downloadEmptyState"
            anchors.fill: parent
            opacity: downloadListView.count > 0 ? 0 : 1
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: GTheme.durationBase
                    easing.type: GTheme.easingStandard
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: GTheme.spaceLG

                // 空状态插图:渐变圆角瓦片 + 下载箭头(替换原破损 no-task.svg),
                // 规格参照 design-system component-polish 的 empty-illustration:
                // 柔和的 primary→success 浅色渐变底 + 主色箭头,跨平台一致、无外部资源依赖
                Rectangle {
                    id: emptyStateTile
                    Layout.preferredWidth: 112
                    Layout.preferredHeight: 88
                    Layout.alignment: Qt.AlignHCenter
                    radius: GTheme.radiusRound
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: GTheme.primaryLight(9) }
                        GradientStop { position: 1.0; color: GTheme.successLight(9) }
                    }

                    FontIcon {
                        anchors.centerIn: parent
                        iconSource: SegoeFluentIcons.Download
                        iconSize: GTheme.fontH1 + GTheme.spaceMD
                        color: GTheme.primaryColor
                    }
                }

                Text {
                    text: {
                        switch(downloadView.pageType) {
                            case 0: return qsTr("No active downloads")
                            case 1: return qsTr("No waiting downloads")
                            case 2: return qsTr("No completed downloads")
                            default: return qsTr("No downloads")
                        }
                    }
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightMedium
                    color: GTheme.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: qsTr("Add some download links to get started")
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textPlaceholder
                    Layout.alignment: Qt.AlignHCenter
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
            spacing: GTheme.spaceSM
            topMargin: GTheme.spaceSM
            bottomMargin: GTheme.spaceLG
            leftMargin: GTheme.space2XL
            rightMargin: GTheme.space2XL
            clip: true
            interactive: true
            orientation: ListView.Vertical
            model: downloadView.model

            delegate: GCard {
                readonly property bool failedTask: model.taskState === downloadView.failedTaskState
                width: downloadListView.width - downloadListView.leftMargin - downloadListView.rightMargin
                height: failedTask ? downloadView.failedTaskCardHeight : downloadView.taskCardHeight
                padding: GTheme.spaceMD
                outlined: true
                hoverEnabled: true
                interactive: true
                radius: GTheme.radiusRound
                variant: "elevated"
                selected: hovered

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceSM
                    // 文件名和操作按钮行
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: GTheme.spaceSM
                        Layout.rightMargin: GTheme.spaceSM
                        spacing: GTheme.spaceLG

                        // 文件类型徽标:不依赖平台图标,使用扩展名文本保持跨平台一致
                        Rectangle {
                            Layout.preferredWidth: downloadView.taskIconSize
                            Layout.preferredHeight: downloadView.taskIconSize
                            Layout.alignment: Qt.AlignVCenter
                            radius: GTheme.radiusMedium
                            color: failedTask ? GTheme.dangerLight(9) : GTheme.primaryLight(9)
                            border.width: 1
                            border.color: failedTask ? GTheme.dangerLight(7) : GTheme.primaryLight(7)

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    const parts = String(model.fileName).split(".")
                                    if (parts.length <= 1) return qsTr("FILE")
                                    return parts[parts.length - 1].slice(0, 3).toUpperCase()
                                }
                                font.pixelSize: GTheme.fontCaption
                                font.weight: GTheme.weightDemiBold
                                color: failedTask ? GTheme.dangerColor : GTheme.primaryColor
                            }
                        }

                        // 文件名区域
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: GTheme.spaceXS

                            Text {
                                text: model.fileName
                                font.pixelSize: GTheme.fontBody
                                font.weight: GTheme.weightMedium
                                color: GTheme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                maximumLineCount: 1
                            }

                            Text {
                                text: downloadView.pageType === 2
                                      ? qsTr("Saved to %1").arg(model.savePath)
                                      : qsTr("%1 of %2 • %3% • %4 left").arg(model.currentSize).arg(model.totalSize).arg(model.progress).arg(model.remainingTime)
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textSecondary
                                Layout.fillWidth: true
                                visible: !failedTask
                            }

                            Text {
                                objectName: "taskErrorDetails"
                                text: {
                                    const code = String(model.errorCode || "").trim()
                                    const message = String(model.errorMessage || "").trim()
                                    if (code.length > 0 && message.length > 0) {
                                        return qsTr("Error %1: %2").arg(code).arg(message)
                                    }
                                    if (message.length > 0) {
                                        return message
                                    }
                                    return qsTr("Download failed")
                                }
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textDanger
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                visible: failedTask
                            }
                        }

                        // 操作按钮区域
                        RowLayout {
                            spacing: GTheme.spaceXS

                            // 开始/恢复按钮
                            GButton {
                                visible: model.taskState !== 1 && !failedTask
                                iconSource: downloadView.pageType === 2 ?
                                           SegoeFluentIcons.OpenFile : SegoeFluentIcons.Play
                                iconSize: GTheme.fontBody
                                Layout.preferredWidth: GTheme.sizeSmall + GTheme.spaceXS
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: {
                                    if (downloadView.pageType === 2) {
                                        Qt.openUrlExternally(model.savePath)
                                    } else if (downloadView.pageType === 0) {
                                        BrowserManager.UnpauseTask(0, model.taskId)
                                    } else if (downloadView.pageType === 1) {
                                        BrowserManager.UnpauseTask(1, model.taskId)
                                    }
                                }
                            }

                            // 失败任务重试
                            GButton {
                                objectName: "btnRetryTask"
                                visible: downloadView.pageType === 2
                                         && failedTask
                                         && String(model.downloadLink || "").trim().length > 0
                                text: qsTr("Retry")
                                buttonType: "primary"
                                iconSource: SegoeFluentIcons.Refresh
                                iconSize: GTheme.fontBody
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: BrowserManager.RetryTask(model.taskId)
                            }

                            // 暂停按钮
                            GButton {
                                visible: model.taskState === 1
                                iconSource: SegoeFluentIcons.Pause
                                iconSize: GTheme.fontBody
                                Layout.preferredWidth: GTheme.sizeSmall + GTheme.spaceXS
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: {
                                    if (downloadView.pageType === 0) {
                                        BrowserManager.PauseTask(0, model.taskId)
                                    } else if (downloadView.pageType === 1) {
                                        BrowserManager.PauseTask(1, model.taskId)
                                    }
                                }
                            }

                            // 分隔线
                            Rectangle {
                                Layout.preferredWidth: 1
                                Layout.preferredHeight: GTheme.spaceLG
                                color: GTheme.borderLight
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // 打开文件夹按钮
                            GButton {
                                iconSource: SegoeFluentIcons.Folder
                                iconSize: GTheme.fontBody
                                Layout.preferredWidth: GTheme.sizeSmall + GTheme.spaceXS
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: {
                                    BrowserManager.OpenFileLocation(model.savePath)
                                }
                            }

                            // 复制链接按钮
                            GButton {
                                iconSource: SegoeFluentIcons.Link
                                iconSize: GTheme.fontBody
                                Layout.preferredWidth: GTheme.sizeSmall + GTheme.spaceXS
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: {
                                    UtilsToolsManager.SetClipboardText(model.downloadLink)
                                    ToastManager.ShowSuccess(qsTr("Link copied to clipboard"))
                                }
                            }

                            // 删除按钮
                            GButton {
                                buttonType: "danger"
                                iconSource: SegoeFluentIcons.Delete
                                iconSize: GTheme.fontBody
                                Layout.preferredWidth: GTheme.sizeSmall + GTheme.spaceXS
                                Layout.preferredHeight: GTheme.sizeSmall + GTheme.spaceXS
                                onClicked: {
                                    // 打开删除确认对话框
                                    deleteConfirmDialog.pageType = downloadView.pageType
                                    deleteConfirmDialog.taskFileName = model.fileName
                                    deleteConfirmDialog.currentTaskId = model.taskId
                                    deleteConfirmDialog.open()
                                }
                            }
                        }
                    }

                    // 进度条
                    GProgressBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 8
                        Layout.leftMargin: GTheme.spaceSM
                        Layout.rightMargin: GTheme.spaceSM
                        from: 0
                        to: 100
                        value: model.progress
                        fgColor: model.taskState === 1 ? GTheme.successColor : GTheme.warningColor
                        status: model.taskState === 1 ? "success" : "warning"
                        visible: downloadView.pageType !== 2  // 已完成任务不显示进度条
                    }

                    // 下载状态信息行
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: GTheme.spaceSM
                        Layout.rightMargin: GTheme.spaceSM
                        Layout.bottomMargin: GTheme.spaceSM
                        spacing: GTheme.spaceLG

                        Text {
                            text: model.currentSize + "/" + model.totalSize
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textSecondary
                            Layout.alignment: Qt.AlignLeft
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // 下载速度
                        Text {
                            text: model.downloadSpeed
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textSuccess
                            visible: downloadView.pageType === 0 && model.taskState === 1
                        }

                        // 剩余时间
                        Text {
                            text: qsTr("Remaining ") + model.remainingTime
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textSecondary
                            visible: downloadView.pageType === 0 && model.taskState === 1
                        }

                        // 连接数
                        RowLayout {
                            spacing: GTheme.spaceXS
                            visible: downloadView.pageType === 0
                            FontIcon {
                                iconSource: SegoeFluentIcons.Connected
                                iconSize: GTheme.fontCaption
                                color: GTheme.textSecondary
                            }

                            Text {
                                text: String("%1").arg(model.connections)
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textSecondary
                            }
                        }
                    }
                }
            }
        }
    }

    // 删除确认对话框
    DeleteConfirmDialog {
        id: deleteConfirmDialog
        parent: Overlay.overlay

        // 存储当前要删除的任务ID
        property string currentTaskId: ""

        // 处理删除操作
        onActionSelected: function(action) {
            if (action === DeleteConfirmDialog.Cancel) {
                // 用户取消，不执行任何操作
                return
            }

            // 确定是否删除文件
            const shouldDeleteFile = (action === DeleteConfirmDialog.DeleteBoth)

            // 根据页面类型调用相应的删除方法
            if (downloadView.pageType === 0) {
                // 正在下载页面
                BrowserManager.RemoveTask(0, currentTaskId, shouldDeleteFile)
            } else if (downloadView.pageType === 1) {
                // 等待中页面（通常没有文件，但保持一致性）
                BrowserManager.RemoveTask(1, currentTaskId, shouldDeleteFile)
            } else if (downloadView.pageType === 2) {
                // 已完成页面
                BrowserManager.RemoveStopTask(currentTaskId, shouldDeleteFile)
            }
        }
    }
}
