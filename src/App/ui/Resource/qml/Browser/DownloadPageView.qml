import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格下载页面视图
Item {
    id: control
    property int currentIndex: 0

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
                type: control.currentIndex
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


            // 下载列表堆栈视图
            StackLayout {
                id: downloadStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: control.currentIndex

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
