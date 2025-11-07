import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 Tracker 服务器设置卡片
GCard {
    id: trackServerPage
    Layout.fillWidth: true
    Layout.preferredHeight: 580
    outlined: true
    padding: 16  // Element Plus 标准内边距

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 卡片标题和描述
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("BitTorrent Tracker Servers")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure tracker servers for BitTorrent downloads")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // Tracker 源选择区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // 子标题
            Text {
                text: qsTr("Tracker Sources")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            // Tracker 服务器选择
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                clip: true
                background: Rectangle {
                    color: GTheme.bgWhite
                    border.color: GTheme.borderLight
                    border.width: 1
                    radius: 4
                }

                GridLayout {
                    id: checkboxGroup
                    width: parent.width
                    columns: 2  // 减少列数以提高可读性
                    columnSpacing: 16
                    rowSpacing: 8

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
                                let pos = checkboxGroup.selectedItems.indexOf(modelData)
                                if (checked) {
                                    if(pos === -1){
                                        checkboxGroup.selectedItems.push(modelData)
                                    }
                                } else {
                                    if(pos !== -1){
                                        removeAllInPlace(checkboxGroup.selectedItems, modelData)
                                    }
                                }
                                let new_names = JSON.stringify(checkboxGroup.selectedItems)
                                SettingsManager.SetTrackerSourceNames(new_names)
                            }

                            function removeAllInPlace(arr, value) {
                                for (var i = arr.length - 1; i >= 0; --i)
                                    if (arr[i] === value) arr.splice(i, 1);
                            }
                        }
                    }
                }
            }
        }

        // 操作按钮区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            GButton {
                id: syncBtn
                type: 1
                text: qsTr("Sync Trackers")
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    console.debug("sync trackers")
                    BrowserManager.SyncTrackersServerlist()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            GButtonSwitch {
                id: autoUpdateSwitch
                checked: SettingsManager.qEnableTrackerSourceAutoUpdate
                text: qsTr("Enable daily auto-update")
                Layout.preferredWidth: 230
                Layout.preferredHeight: 36
                font.pixelSize: 14
                onClicked: {
                    SettingsManager.SetEnableTrackerSourceAutoUpdate(checked)
                }
            }
        }

        // Tracker 更新状态显示区域
        RowLayout {
            id: statusArea
            Layout.fillWidth: true
            spacing: 12
            visible: statusText.text !== ""

            Rectangle {
                width: 4
                height: parent.height
                radius: 2
                color: {
                    if (statusText.status === "started") return GTheme.warningColor
                    if (statusText.status === "success") return GTheme.successColor
                    if (statusText.status === "error") return GTheme.dangerColor
                    return GTheme.infoColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    id: statusText
                    property string status: ""
                    font.pixelSize: 13
                    color: {
                        if (status === "started") return GTheme.warningColor
                        if (status === "success") return GTheme.successColor
                        if (status === "error") return GTheme.dangerColor
                        return GTheme.textPrimary
                    }
                }

                Text {
                    id: statusDetails
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    visible: text !== ""
                }
            }
        }

        // Tracker 列表预览区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Current Tracker List")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            ScrollView {
                id: serverResultScrollView
                Layout.fillWidth: true
                Layout.preferredHeight: 150
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
                        radius: 4
                    }
                }
            }
        }
    }

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

                    // 13秒后自动隐藏成功消息
                    hideTimer.restart()
                } else if (data.status === "error") {
                    statusText.text = data.message || qsTr("Failed to update tracker list")
                    statusDetails.text = data.error || ""
                    syncBtn.enabled = true

                    // 15秒后自动隐藏错误消息
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
            interval = 13000  // 重置为默认值
        }
    }
}
