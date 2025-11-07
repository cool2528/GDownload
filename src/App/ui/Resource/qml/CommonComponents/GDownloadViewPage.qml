import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Element Plus 风格下载列表页面
Control {
    id: downloadView
    property alias model: downloadListView.model
    property int pageType: -1 // 0 downloadPage 1 waitingPage  2 completedPage

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int cardSpacing: 12
    readonly property int contentMargin: 24

    background: Rectangle {
        color: GTheme.bgPage

        // 空状态显示
        Item {
            anchors.fill: parent
            opacity: downloadListView.count > 0 ? 0 : 1
            visible: opacity > 0

            Behavior on opacity {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: downloadView.standardSpacing

                Image {
                    id: emptyStateImage
                    source: "/images/browser/no-task.svg"
                    Layout.alignment: Qt.AlignHCenter
                    sourceSize.width: 120
                    sourceSize.height: 120
                    opacity: 0.6
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
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    color: GTheme.textSecondary
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: qsTr("Add some download links to get started")
                    font.pixelSize: 14
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
            spacing: downloadView.cardSpacing
            topMargin: downloadView.standardSpacing
            bottomMargin: downloadView.standardSpacing
            leftMargin: downloadView.contentMargin
            rightMargin: downloadView.contentMargin
            clip: true
            interactive: true
            orientation: ListView.Vertical
            model: downloadView.model

            delegate: GCard {
                width: downloadListView.width - downloadListView.leftMargin - downloadListView.rightMargin
                height: 120
                padding: downloadView.standardSpacing
                outlined: true
                hoverEnabled: true
                selected: hovered

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8
                    // 文件名和操作按钮行
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        spacing: downloadView.standardSpacing

                        // 文件名区域
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: model.fileName
                                font.pixelSize: 15
                                font.weight: Font.Medium
                                color: GTheme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                maximumLineCount: 1
                            }

                            Text {
                                visible: pageType !== 2
                                text: qsTr("Size: %1 • Progress: %2%").arg(model.totalSize).arg(model.progress)
                                font.pixelSize: 12
                                color: GTheme.textSecondary
                                Layout.fillWidth: true
                            }
                        }

                        // 操作按钮区域
                        RowLayout {
                            spacing: 4

                            // 开始/恢复按钮
                            IconButton {
                                visible: model.taskState !== 1
                                iconSource: downloadView.pageType === 2 ?
                                           SegoeFluentIcons.OpenFile : SegoeFluentIcons.Play
                                iconSize: 14
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                                backgroundColor: hovered ? GTheme.fillLight : "transparent"
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

                            // 暂停按钮
                            IconButton {
                                visible: model.taskState === 1
                                iconSource: SegoeFluentIcons.Pause
                                iconSize: 14
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                                backgroundColor: hovered ? GTheme.fillLight : "transparent"
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
                                Layout.preferredHeight: 16
                                color: GTheme.borderLight
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // 打开文件夹按钮
                            IconButton {
                                iconSource: SegoeFluentIcons.Folder
                                iconSize: 14
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                                onClicked: {
                                    BrowserManager.OpenFileLocation(model.savePath)
                                }
                            }

                            // 复制链接按钮
                            IconButton {
                                iconSource: SegoeFluentIcons.Link
                                iconSize: 14
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                iconColor: hovered ? GTheme.primaryColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                                onClicked: {
                                    UtilsToolsManager.SetClipboardText(model.downloadLink)
                                    ToastManager.ShowSuccess(qsTr("Link copied to clipboard"))
                                }
                            }

                            // 删除按钮
                            IconButton {
                                iconSource: SegoeFluentIcons.Delete
                                iconSize: 14
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                iconColor: hovered ? GTheme.dangerColor : (GTheme.dark ? GTheme.textPrimary : GTheme.textSecondary)
                                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                                onClicked: {
                                    if (downloadView.pageType === 0) {
                                        BrowserManager.RemoveTask(0, model.taskId)
                                    } else if (downloadView.pageType === 1) {
                                        BrowserManager.RemoveTask(1, model.taskId)
                                    } else if (downloadView.pageType === 2) {
                                        BrowserManager.RemoveStopTask(model.taskId)
                                    }
                                }
                            }
                        }
                    }

                    // 进度条
                    GProgressBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        from: 0
                        to: 100
                        value: model.progress
                        visible: downloadView.pageType !== 2  // 已完成任务不显示进度条
                    }

                    // 下载状态信息行
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.bottomMargin: 8
                        spacing: downloadView.standardSpacing

                        Text {
                            text: model.currentSize + "/" + model.totalSize
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                            Layout.alignment: Qt.AlignLeft
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        // 下载速度
                        Text {
                            text: model.downloadSpeed
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                            visible: downloadView.pageType === 0 && model.taskState === 1
                        }

                        // 剩余时间
                        Text {
                            text: qsTr("Remaining ") + model.remainingTime
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                            visible: downloadView.pageType === 0 && model.taskState === 1
                        }

                        // 连接数
                        RowLayout {
                            spacing: 4
                            visible: downloadView.pageType === 0
                            FontIcon {
                                iconSource: SegoeFluentIcons.Connected
                                iconSize: 12
                                color: GTheme.textSecondary
                            }

                            Text {
                                text: String("%1").arg(model.connections)
                                font.pixelSize: 12
                                color: GTheme.textSecondary
                            }
                        }
                    }
                }
            }
        }
    }
}
