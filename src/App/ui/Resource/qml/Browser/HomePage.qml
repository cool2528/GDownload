import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// 主页 / 概览仪表盘:欢迎区 + 状态概览 + 快速开始 + 最近活动
// 全部使用真实数据(BrowserManager 三个模型计数 + 已完成列表),不编造无数据源的统计
Item {
    id: control

    // 任务模型(测试桩环境下返回 null,计数为 0,页面正常渲染)
    property var activeModel: BrowserManager.GetActiveDownloadModel()
    property var waitingModel: BrowserManager.GetWaitingDownloadModel()
    property var completedModel: BrowserManager.GetStopedDownloadModel()

    // 页面级布局常量
    readonly property int contentMaxWidth: 1080
    readonly property int recentRowHeight: GTheme.sizeLarge + GTheme.spaceSM
    readonly property bool compactLayout: width < 760
    readonly property int pageInset: compactLayout ? GTheme.spaceLG : GTheme.space2XL

    // 快捷入口复用 TaskDialogPage,不新增第二套任务创建 UI
    Component { id: taskDialogComponent; TaskDialogPage {} }

    // 打开新建任务弹窗并预选标签(0=URL 1=Torrent 2=eD2k 3=Cloud Drive)
    function openTaskDialog(tab) {
        let parentItem = (typeof mainWindow !== "undefined" && mainWindow) ? mainWindow : control
        let task = taskDialogComponent.createObject(parentItem, { initialTab: tab })
        if (task === null) {
            console.error("Error creating task dialog")
            return
        }
        task.closed.connect(function() { task.destroy() })
        task.open()
    }

    // 计数改为直接绑定 C++ 模型的 count 属性(NOTIFY 驱动),移除隐藏 Repeater(P4)
    readonly property int activeCount: control.activeModel ? control.activeModel.count : 0
    readonly property int waitingCount: control.waitingModel ? control.waitingModel.count : 0
    readonly property int completedCount: control.completedModel ? control.completedModel.count : 0

    Rectangle {
        anchors.fill: parent
        color: GTheme.bgPage

        ScrollView {
            id: homeScroll
            anchors.fill: parent
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip: true

            ColumnLayout {
                width: Math.min(control.contentMaxWidth,
                                Math.max(0, homeScroll.availableWidth - control.pageInset * 2))
                x: Math.max(control.pageInset, (homeScroll.availableWidth - width) / 2)
                y: control.pageInset
                spacing: GTheme.spaceLG

                // ===== 欢迎区 =====
                GridLayout {
                    Layout.fillWidth: true
                    columns: control.compactLayout ? 1 : 2
                    columnSpacing: GTheme.spaceLG
                    rowSpacing: GTheme.spaceMD

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: GTheme.spaceXS

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Welcome back")
                            font.pixelSize: GTheme.fontH1
                            font.weight: GTheme.weightDemiBold
                            color: GTheme.textPrimary
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Monitor transfers, start a task, or reopen a completed file.")
                            font.pixelSize: GTheme.fontBody
                            color: GTheme.textSecondary
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }

                    GButton {
                        objectName: "homeAddDownloadButton"
                        Layout.fillWidth: control.compactLayout
                        Layout.alignment: control.compactLayout ? Qt.AlignLeft : Qt.AlignRight
                        text: qsTr("Add Download")
                        iconName: "add"
                        size: "large"
                        buttonType: "primary"
                        onClicked: control.openTaskDialog(0)
                    }
                }

                // ===== 状态概览 =====
                GridLayout {
                    id: summaryGrid
                    Layout.fillWidth: true
                    columns: control.compactLayout ? 1 : 3
                    columnSpacing: GTheme.spaceMD
                    rowSpacing: GTheme.spaceMD

                    SummaryMetricCard {
                        objectName: "homeActiveMetric"
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        expanded: true
                        title: qsTr("Active")
                        value: String(control.activeCount)
                        unit: qsTr("tasks")
                        detail: qsTr("Transfers currently running")
                        iconName: "download"
                        accent: "primary"
                    }
                    SummaryMetricCard {
                        objectName: "homeWaitingMetric"
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        expanded: true
                        title: qsTr("Waiting")
                        value: String(control.waitingCount)
                        unit: qsTr("tasks")
                        detail: qsTr("Queued for the next slot")
                        iconName: "history"
                        accent: "warning"
                    }
                    SummaryMetricCard {
                        objectName: "homeCompletedMetric"
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        expanded: true
                        title: qsTr("Completed")
                        value: String(control.completedCount)
                        unit: qsTr("tasks")
                        detail: qsTr("Ready to open again")
                        iconName: "completed"
                        accent: "success"
                    }
                }

                // ===== 快速开始 =====
                SettingCard {
                    Layout.fillWidth: true
                    title: qsTr("Quick start")
                    description: qsTr("Add a new download from a link, torrent, or cloud drive share.")

                    GridLayout {
                        Layout.fillWidth: true
                        columns: control.compactLayout ? 1 : 3
                        columnSpacing: GTheme.spaceMD
                        rowSpacing: GTheme.spaceMD

                        QuickActionCard {
                            objectName: "homeAddUrlAction"
                            Layout.fillWidth: true
                            title: qsTr("Add URL")
                            description: qsTr("Paste download links")
                            iconName: "globe"
                            accent: "primary"
                            onClicked: control.openTaskDialog(0)
                        }
                        QuickActionCard {
                            objectName: "homeAddTorrentAction"
                            Layout.fillWidth: true
                            title: qsTr("Torrent")
                            description: qsTr("Drop torrent files")
                            iconName: "cloud-download"
                            accent: "success"
                            onClicked: control.openTaskDialog(1)
                        }
                        QuickActionCard {
                            objectName: "homeAddCloudAction"
                            Layout.fillWidth: true
                            title: qsTr("Cloud Drive")
                            description: qsTr("Parse cloud links")
                            iconName: "cloud"
                            accent: "warning"
                            onClicked: control.openTaskDialog(3)
                        }
                    }
                }

                // ===== 最近活动(已完成) =====
                SettingCard {
                    Layout.fillWidth: true
                    title: qsTr("Recent activity")
                    description: qsTr("Your most recently completed downloads.")

                    // 空状态
                    EmptyState {
                        Layout.fillWidth: true
                        visible: control.completedCount === 0
                        compact: true
                        iconName: "completed"
                        title: qsTr("No completed downloads yet")
                        description: qsTr("Completed files will appear here for quick access.")
                        actionText: qsTr("Add download")
                        onActionTriggered: control.openTaskDialog(0)
                    }

                    // 最近完成列表(最多 5 条)
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(5, control.completedCount) * control.recentRowHeight
                        visible: control.completedCount > 0
                        interactive: false
                        clip: true
                        model: control.completedModel

                        delegate: Item {
                            width: ListView.view.width
                            height: control.recentRowHeight
                            visible: index < 5

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: GTheme.spaceXS
                                anchors.rightMargin: GTheme.spaceXS
                                spacing: GTheme.spaceSM

                                Rectangle {
                                    Layout.preferredWidth: GTheme.sizeDefault
                                    Layout.preferredHeight: GTheme.sizeDefault
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: GTheme.radiusMedium
                                    color: GTheme.bgSuccess

                                    AuroraIcon {
                                        anchors.centerIn: parent
                                        name: "completed"
                                        iconSize: GTheme.fontBody
                                        color: GTheme.successColor
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: model.fileName
                                    font.pixelSize: GTheme.fontBody
                                    color: GTheme.textPrimary
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }

                                GButton {
                                    variant: "chip"
                                    text: qsTr("Open")
                                    Layout.preferredHeight: GTheme.sizeSmall
                                    onClicked: BrowserManager.OpenFileLocation(model.savePath)
                                }
                            }
                        }
                    }
                }

                // 底部留白
                Item { Layout.preferredHeight: GTheme.space2XL }
            }
        }
    }
}
