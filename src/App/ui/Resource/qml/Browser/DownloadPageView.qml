import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格下载页面视图
Item {
    id: control
    property alias currentIndex: navigationBar.currentIndex

    // 快捷入口复用现有 TaskDialogPage,不新增第二套任务创建 UI
    Component { id: taskDialogComponent; TaskDialogPage {} }

    function openTaskDialog() {
        let parentItem = typeof mainWindow !== "undefined" && mainWindow ? mainWindow : control
        let task = taskDialogComponent.createObject(parentItem)
        if (task === null) {
            console.error("Error creating task dialog")
            return
        }
        task.closed.connect(function(){ task.destroy() })
        task.open()
    }

    Rectangle {
        id: mainContent
        anchors.fill: parent
        color: GTheme.bgPage

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 页面标题栏
            DownloadPageTitle {
                id: downloadTitle
                Layout.fillWidth: true
                type: navigationBar.currentIndex
                onAddTaskRequested: control.openTaskDialog()
            }

            // 下载中心摘要条:紧凑展示当前分类计数与密度模式
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeLarge + GTheme.spaceSM * 4
                Layout.leftMargin: GTheme.space2XL
                Layout.rightMargin: GTheme.space2XL
                Layout.topMargin: GTheme.spaceMD
                Layout.bottomMargin: GTheme.spaceSM
                padding: GTheme.spaceSM
                outlined: true
                hoverEnabled: false
                variant: "elevated"

                RowLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceSM

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Active")
                        value: String(downloadPage.taskCount)
                        unit: qsTr("tasks")
                        iconSource: SegoeFluentIcons.Download
                        accent: "primary"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Waiting")
                        value: String(waitingPage.taskCount)
                        unit: qsTr("tasks")
                        iconSource: SegoeFluentIcons.History
                        accent: "warning"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Stopped")
                        value: String(completedPage.taskCount)
                        unit: qsTr("tasks")
                        iconSource: SegoeFluentIcons.Completed
                        accent: "success"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Density")
                        value: qsTr("Comfort")
                        unit: ""
                        iconSource: SegoeFluentIcons.View
                        accent: "info"
                    }
                }
            }

            // V5 分类筛选:替代旧二级侧栏,保留 currentIndex alias 给 BrowserView 使用
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeLarge + GTheme.spaceSM
                Layout.leftMargin: GTheme.space2XL
                Layout.rightMargin: GTheme.space2XL
                Layout.bottomMargin: GTheme.spaceSM
                padding: GTheme.spaceXS
                outlined: true
                hoverEnabled: false
                variant: "muted"

                RowLayout {
                    id: navigationBar
                    anchors.fill: parent
                    spacing: GTheme.spaceXS
                    property int currentIndex: 0
                    property var buttonsArr: [downloadingBtn, waitingBtn, stoppedBtn]

                    onCurrentIndexChanged: {
                        const button = buttonsArr[currentIndex]
                        if (button && !button.checked) {
                            button.checked = true
                        }
                    }

                    ButtonGroup {
                        id: navigationGroup
                        onCheckedButtonChanged: {
                            let index = navigationBar.buttonsArr.indexOf(navigationGroup.checkedButton)
                            if (index >= 0) {
                                navigationBar.currentIndex = index
                            }
                        }
                    }

                    GButton {
                        id: downloadingBtn
                        variant: "nav"
                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.navItemHeight
                        checkable: true
                        checked: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.PlaySolid
                        text: qsTr("Active")
                        onClicked: checked = true
                    }

                    GButton {
                        id: waitingBtn
                        variant: "nav"
                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.navItemHeight
                        checkable: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.History
                        text: qsTr("Waiting")
                        onClicked: checked = true
                    }

                    GButton {
                        id: stoppedBtn
                        objectName: "navCompleted"
                        variant: "nav"
                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.navItemHeight
                        checkable: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.Stop
                        text: qsTr("Stopped")
                        onClicked: checked = true
                    }
                }
            }

            // 快捷入口:当前批次打开现有 TaskDialog,初始 tab 路由留到 TaskDialog 批次
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeLarge + GTheme.spaceSM * 2
                Layout.leftMargin: GTheme.space2XL
                Layout.rightMargin: GTheme.space2XL
                Layout.bottomMargin: GTheme.spaceMD
                spacing: GTheme.spaceSM

                QuickActionCard {
                    Layout.fillWidth: true
                    title: qsTr("URL")
                    description: qsTr("Paste download links")
                    iconSource: SegoeFluentIcons.Link
                    accent: "primary"
                    onClicked: control.openTaskDialog()
                }

                QuickActionCard {
                    Layout.fillWidth: true
                    title: qsTr("Torrent")
                    description: qsTr("Drop torrent files")
                    iconSource: SegoeFluentIcons.CloudDownload
                    accent: "success"
                    onClicked: control.openTaskDialog()
                }

                QuickActionCard {
                    Layout.fillWidth: true
                    title: qsTr("Baidu")
                    description: qsTr("Parse cloud links")
                    iconSource: SegoeFluentIcons.Cloud
                    accent: "warning"
                    onClicked: control.openTaskDialog()
                }
            }

            // 下载列表堆栈视图
            StackLayout {
                id: downloadStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: navigationBar.currentIndex

                // 下载中页面
                GDownloadViewPage {
                    id: downloadPage
                    pageType: 0
                    objectName: "downloadPage"
                    model: BrowserManager.GetActiveDownloadModel()
                }

                // 等待中页面
                GDownloadViewPage {
                    id: waitingPage
                    pageType: 1
                    objectName: "waitingPage"
                    model: BrowserManager.GetWaitingDownloadModel()
                }

                // 已停止页面
                GDownloadViewPage {
                    id: completedPage
                    pageType: 2
                    objectName: "completedPage"
                    model: BrowserManager.GetStopedDownloadModel()
                }
            }
        }
    }
}
