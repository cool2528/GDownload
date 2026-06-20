import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 Tracker 服务器设置卡片(B 类,即时提交派)
// Tracker 源勾选与自动更新开关直接 SetXxx 提交,同步按钮调用 BrowserManager;
// 骨架走 SettingCard;设置项行走 SettingRow;分隔线统一 Divider;尺寸/间距/字号/圆角全走 GTheme 令牌
SettingCard {
    id: trackServerPage

    // 父容器(AdvancedSettingPage)已设 Layout.fillWidth/margins;此处保留 fillWidth 以兼容独立使用
    Layout.fillWidth: true

    title: qsTr("BitTorrent Tracker Servers")
    description: qsTr("Configure tracker servers for BitTorrent downloads")

    // ========== Tracker 源选择区域 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Tracker Sources")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        // Tracker 服务器选择
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault * 6 + GTheme.spaceSM
            clip: true
            background: Rectangle {
                color: GTheme.bgWhite
                border.color: GTheme.borderLight
                border.width: 1
                radius: GTheme.radiusBase
            }

            GridLayout {
                id: checkboxGroup
                width: parent.width
                columns: 2  // 两列布局提升可读性(布局业务值,非尺寸令牌)
                columnSpacing: GTheme.spaceLG
                rowSpacing: GTheme.spaceSM

                property var selectedItems: JSON.parse(SettingsManager.qTrackerSourceNames)

                Repeater {
                    model: ["ngosang-best-link","ngosang-best-mirror","ngosang-best-cdn","ngosang-all-link","ngosang-all-mirror","ngosang-all-cdn",
                        "ngosang-all_udp-link","ngosang-all_udp-mirror","ngosang-all_udp-cdn",
                        "ngosang-all_http-link","ngosang-all_http-mirror","ngosang-all_http-cdn",
                        "ngosang-all_https-link","ngosang-all_https-mirror","ngosang-all_https-cdn",
                        "XIU2-best-link","XIU2-best-cdn","XIU2-all-link","XIU2-all-cdn","XIU2-http-link","XIU2-http-cdn","XIU2-nohttp-link","XIU2-nohttp-cdn"]

                    GCheckBox {
                        text: modelData
                        checked: checkboxGroup.selectedItems.indexOf(modelData) !== -1
                        Layout.fillWidth: true
                        onClicked: {
                            // 原地 push/splice 修改 property var 数组不触发变更信号，
                            // 改用 concat/filter 返回新数组重新赋值，确保 UI 同步。
                            let items = checkboxGroup.selectedItems
                            let pos = items.indexOf(modelData)
                            let newItems
                            if (checked) {
                                newItems = pos === -1 ? items.concat(modelData) : items
                            } else {
                                newItems = pos !== -1 ? items.filter(i => i !== modelData) : items
                            }
                            checkboxGroup.selectedItems = newItems
                            // 点击会破坏 checked 绑定，重建以保持与 selectedItems 同步
                            checked = Qt.binding(function() { return checkboxGroup.selectedItems.indexOf(modelData) !== -1 })
                            let new_names = JSON.stringify(newItems)
                            SettingsManager.SetTrackerSourceNames(new_names)
                        }
                    }
                }
            }
        }
    }

    // 分隔线(统一 Divider,色 borderLight)
    Divider {
        Layout.fillWidth: true
    }

    // ========== 操作区域:即时提交 + 手动同步 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            GButton {
                id: syncBtn
                type: 1
                text: qsTr("Sync Trackers")
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    console.debug("sync trackers")
                    BrowserManager.SyncTrackersServerlist()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Auto Update:")
            hint: qsTr("Enable daily tracker source synchronization")
            control: GButtonSwitch {
                id: autoUpdateSwitch
                checked: SettingsManager.qEnableTrackerSourceAutoUpdate
                text: ""
                Layout.preferredHeight: GTheme.sizeDefault
                font.pixelSize: GTheme.fontBody
                onClicked: {
                    SettingsManager.SetEnableTrackerSourceAutoUpdate(checked)
                }
            }
        }
    }

    // Tracker 更新状态显示区域
    RowLayout {
        id: statusArea
        Layout.fillWidth: true
        spacing: GTheme.spaceMD
        visible: statusText.text !== ""

        // 状态指示条:宽度 radiusBase,纵向跟随状态文本高度
        Rectangle {
            Layout.preferredWidth: GTheme.radiusBase
            Layout.fillHeight: true
            radius: GTheme.radiusSmall
            color: {
                if (statusText.status === "started") return GTheme.warningColor
                if (statusText.status === "success") return GTheme.successColor
                if (statusText.status === "error") return GTheme.dangerColor
                return GTheme.infoColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                id: statusText
                property string status: ""
                font.pixelSize: GTheme.fontCaption
                color: {
                    if (status === "started") return GTheme.warningColor
                    if (status === "success") return GTheme.successColor
                    if (status === "error") return GTheme.dangerColor
                    return GTheme.textPrimary
                }
                wrapMode: Text.WordWrap
            }

            Text {
                id: statusDetails
                Layout.fillWidth: true
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                visible: text !== ""
                wrapMode: Text.WordWrap
            }
        }
    }

    // 分隔线(统一 Divider,色 borderLight)
    Divider {
        Layout.fillWidth: true
    }

    // ========== Tracker 列表预览区域 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceSM

        Text {
            Layout.fillWidth: true
            text: qsTr("Current Tracker List")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        ScrollView {
            id: serverResultScrollView
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault * 5
            clip: true

            TextArea {
                id: serverResult
                text: UtilsToolsManager.serverList
                color: GTheme.textPrimary
                placeholderTextColor: GTheme.textPlaceholder
                selectByMouse: true
                readOnly: true
                wrapMode: TextArea.Wrap
                background: Rectangle {
                    color: GTheme.fillLighter
                    border.color: GTheme.borderLight
                    border.width: 1
                    radius: GTheme.radiusBase
                }
            }
        }
    }

    // 非可视逻辑对象:避免 SettingCard 的 content 槽只接收 Item 时丢失 Connections/Timer
    Item {
        Layout.preferredWidth: 0
        Layout.preferredHeight: 0
        visible: false

        // 监听 Tracker 更新状态
        Connections {
            target: BrowserManager

            function onSigTrackerUpdateStatus(status) {
                try {
                    var data = JSON.parse(status)
                    statusText.status = data.status

                    if (data.status === "started") {
                        statusText.text = data.message || qsTr("Updating tracker list...")
                        statusDetails.text = ""
                        syncBtn.enabled = false
                    } else if (data.status === "success") {
                        statusText.text = data.message || qsTr("Tracker list updated successfully")
                        statusDetails.text = qsTr("%1 trackers, %2/%3 sources succeeded, took %4ms")
                            .arg(data.tracker_count || 0)
                            .arg(data.successful_sources || 0)
                            .arg((data.successful_sources || 0) + (data.failed_sources || 0))
                            .arg(data.elapsed_ms || 0)
                        syncBtn.enabled = true

                        // 13 秒后自动隐藏成功消息(业务时间值)
                        hideTimer.restart()
                    } else if (data.status === "error") {
                        statusText.text = data.message || qsTr("Failed to update tracker list")
                        statusDetails.text = data.error || ""
                        syncBtn.enabled = true

                        // 15 秒后自动隐藏错误消息(业务时间值)
                        hideTimer.interval = 15000
                        hideTimer.restart()
                    }
                } catch (e) {
                    console.error("Failed to parse tracker update status:", e)
                }
            }
        }

        // 自动隐藏成功/错误消息的计时器
        Timer {
            id: hideTimer
            interval: 13000
            repeat: false
            onTriggered: {
                statusText.text = ""
                statusDetails.text = ""
                statusText.status = ""
                interval = 13000  // 重置为默认业务时间值
            }
        }
    }
}
