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

    // 快捷入口复用 TaskDialogPage,不新增第二套任务创建 UI
    Component { id: taskDialogComponent; TaskDialogPage {} }

    // 打开新建任务弹窗并预选标签(0=URL 1=Torrent 2=Baidu)
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

    // 隐藏计数器:绑定模型读取 count(无可视开销)
    Repeater { id: activeCounter; model: control.activeModel; delegate: Item {} }
    Repeater { id: waitingCounter; model: control.waitingModel; delegate: Item {} }
    Repeater { id: completedCounter; model: control.completedModel; delegate: Item {} }

    Rectangle {
        anchors.fill: parent
        color: GTheme.bgPage

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip: true

            ColumnLayout {
                width: Math.min(control.contentMaxWidth, parent.width - GTheme.space2XL * 2)
                x: Math.max(GTheme.space2XL, (parent.width - width) / 2)
                y: GTheme.space2XL
                spacing: GTheme.spaceLG

                // ===== 欢迎区 =====
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceXS

                    Text {
                        text: qsTr("Welcome back")
                        font.pixelSize: GTheme.fontH1
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    Text {
                        text: qsTr("Manage your downloads and start new tasks here.")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                    }
                }

                // ===== 状态概览 =====
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeLarge + GTheme.spaceSM * 4
                    padding: GTheme.spaceSM
                    outlined: true
                    hoverEnabled: false
                    variant: "elevated"
                    shadow: true

                    RowLayout {
                        anchors.fill: parent
                        spacing: GTheme.spaceSM

                        SummaryMetricCard {
                            Layout.fillWidth: true
                            title: qsTr("Active")
                            value: String(activeCounter.count)
                            unit: qsTr("tasks")
                            iconSource: SegoeFluentIcons.Download
                            accent: "primary"
                        }
                        SummaryMetricCard {
                            Layout.fillWidth: true
                            title: qsTr("Waiting")
                            value: String(waitingCounter.count)
                            unit: qsTr("tasks")
                            iconSource: SegoeFluentIcons.History
                            accent: "warning"
                        }
                        SummaryMetricCard {
                            Layout.fillWidth: true
                            title: qsTr("Completed")
                            value: String(completedCounter.count)
                            unit: qsTr("tasks")
                            iconSource: SegoeFluentIcons.Completed
                            accent: "success"
                        }
                    }
                }

                // ===== 快速开始 =====
                SettingCard {
                    Layout.fillWidth: true
                    title: qsTr("Quick start")
                    description: qsTr("Add a new download from a link, torrent, or Baidu cloud share.")

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceMD

                        QuickActionCard {
                            Layout.fillWidth: true
                            title: qsTr("Add URL")
                            description: qsTr("Paste download links")
                            iconSource: SegoeFluentIcons.Globe
                            accent: "primary"
                            onClicked: control.openTaskDialog(0)
                        }
                        QuickActionCard {
                            Layout.fillWidth: true
                            title: qsTr("Torrent")
                            description: qsTr("Drop torrent files")
                            iconSource: SegoeFluentIcons.CloudDownload
                            accent: "success"
                            onClicked: control.openTaskDialog(1)
                        }
                        QuickActionCard {
                            Layout.fillWidth: true
                            title: qsTr("Baidu")
                            description: qsTr("Parse cloud links")
                            iconSource: SegoeFluentIcons.Cloud
                            accent: "warning"
                            onClicked: control.openTaskDialog(2)
                        }
                    }
                }

                // ===== 最近活动(已完成) =====
                SettingCard {
                    Layout.fillWidth: true
                    title: qsTr("Recent activity")
                    description: qsTr("Your most recently completed downloads.")

                    // 空状态
                    Text {
                        Layout.fillWidth: true
                        visible: completedCounter.count === 0
                        text: qsTr("No completed downloads yet.")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textPlaceholder
                    }

                    // 最近完成列表(最多 5 条)
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(5, completedCounter.count) * control.recentRowHeight
                        visible: completedCounter.count > 0
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

                                    FontIcon {
                                        anchors.centerIn: parent
                                        iconSource: SegoeFluentIcons.Completed
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
