import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格下载页面视图
Item {
    id: control
    property int currentIndex: 0
    readonly property bool compactLayout: width < 760
    readonly property int pagePadding: compactLayout ? GTheme.spaceSM : GTheme.space2XL

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

    function refreshCurrentDownloadPage() {
        if (control.currentIndex === 0) {
            downloadPage.refreshLayout()
        } else if (control.currentIndex === 1) {
            waitingPage.refreshLayout()
        } else if (control.currentIndex === 2) {
            completedPage.refreshLayout()
        }
    }

    onCurrentIndexChanged: Qt.callLater(control.refreshCurrentDownloadPage)
    Component.onCompleted: Qt.callLater(control.refreshCurrentDownloadPage)

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

            // 下载中心摘要条：窄宽改为两列，避免四张指标卡互相遮挡。
            GCard {
                objectName: "downloadSummaryCard"
                Layout.fillWidth: true
                Layout.preferredHeight: summaryGrid.implicitHeight + padding * 2
                Layout.leftMargin: control.pagePadding
                Layout.rightMargin: control.pagePadding
                Layout.topMargin: GTheme.spaceMD
                Layout.bottomMargin: GTheme.spaceSM
                padding: GTheme.spaceSM
                outlined: true
                hoverEnabled: false
                variant: "elevated"

                GridLayout {
                    id: summaryGrid
                    objectName: "downloadSummaryGrid"
                    anchors.fill: parent
                    columns: control.compactLayout ? 2 : 4
                    columnSpacing: GTheme.spaceSM
                    rowSpacing: GTheme.spaceSM

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Active")
                        value: String(downloadPage.taskCount)
                        unit: qsTr("tasks")
                        iconName: "download"
                        accent: "primary"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Waiting")
                        value: String(waitingPage.taskCount)
                        unit: qsTr("tasks")
                        iconName: "queue"
                        accent: "warning"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Stopped")
                        value: String(completedPage.taskCount)
                        unit: qsTr("tasks")
                        iconName: "completed"
                        accent: "success"
                    }

                    SummaryMetricCard {
                        Layout.fillWidth: true
                        title: qsTr("Density")
                        value: qsTr("Comfort")
                        unit: ""
                        iconName: "view"
                        accent: "info"
                    }
                }
            }


            // 下载列表堆栈视图
            Item {
                id: downloadStack
                Layout.fillWidth: true
                Layout.fillHeight: true

                // 下载中页面
                GDownloadViewPage {
                    id: downloadPage
                    anchors.fill: parent
                    visible: control.currentIndex === 0
                    enabled: control.currentIndex === 0
                    z: control.currentIndex === 0 ? 1 : 0
                    pageType: 0
                    objectName: "downloadPage"
                    model: BrowserManager.GetActiveDownloadModel()
                }

                // 等待中页面
                GDownloadViewPage {
                    id: waitingPage
                    anchors.fill: parent
                    visible: control.currentIndex === 1
                    enabled: control.currentIndex === 1
                    z: control.currentIndex === 1 ? 1 : 0
                    pageType: 1
                    objectName: "waitingPage"
                    model: BrowserManager.GetWaitingDownloadModel()
                }

                // 已停止页面
                GDownloadViewPage {
                    id: completedPage
                    anchors.fill: parent
                    visible: control.currentIndex === 2
                    enabled: control.currentIndex === 2
                    z: control.currentIndex === 2 ? 1 : 0
                    pageType: 2
                    objectName: "completedPage"
                    model: BrowserManager.GetStopedDownloadModel()
                }
            }
        }
    }
}
