import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 Tracker 服务器设置卡片(B 类,即时提交派)
// 源按提供方分组展示逻辑源(镜像通道由引擎自动回退,不再暴露给用户);
// 支持用户自定义源 URL 增删;勾选与自动更新开关直接 SetXxx 提交,同步按钮调用 BrowserManager;
// 旧版「源×通道」配置名(-link/-mirror/-cdn)在展示与保存时归一化为逻辑源名;
// 骨架走 SettingCard;设置项行走 SettingRow;分隔线统一 Divider;尺寸/间距/字号/圆角全走 GTheme 令牌
SettingCard {
    id: trackServerPage

    // 父容器(AdvancedSettingPage)已设 Layout.fillWidth/margins;此处保留 fillWidth 以兼容独立使用
    Layout.fillWidth: true

    title: qsTr("BitTorrent Tracker Servers")
    description: qsTr("Choose tracker lists to merge into aria2c. Mirrors are tried automatically when a source is unreachable.")

    // ========== 源分组模型(与 aria2c_manager 的 TrackerSourceMirrors 逻辑源名保持一致) ==========
    readonly property var sourceGroups: [
        {
            provider: "ngosang / trackerslist",
            hint: qsTr("Curated public tracker lists, updated daily on GitHub"),
            sources: [
                { key: "ngosang-best", name: qsTr("Best"), desc: qsTr("About 20 stable and fast trackers"), recommended: true },
                { key: "ngosang-best-ip", name: qsTr("Best (IP)"), desc: qsTr("IP-address version, useful when DNS is blocked"), recommended: false },
                { key: "ngosang-all", name: qsTr("All"), desc: qsTr("Full list of working trackers"), recommended: false },
                { key: "ngosang-all_udp", name: qsTr("UDP only"), desc: qsTr("Only udp:// trackers"), recommended: false },
                { key: "ngosang-all_http", name: qsTr("HTTP only"), desc: qsTr("Only http:// trackers"), recommended: false },
                { key: "ngosang-all_https", name: qsTr("HTTPS only"), desc: qsTr("Only https:// trackers"), recommended: false }
            ]
        },
        {
            provider: "XIU2 / TrackersListCollection",
            hint: qsTr("Popular tracker collection with China-friendly mirrors, updated daily"),
            sources: [
                { key: "XIU2-best", name: qsTr("Best"), desc: qsTr("About 20 popular and reliable trackers"), recommended: true },
                { key: "XIU2-all", name: qsTr("All"), desc: qsTr("Aggregated list of all working trackers"), recommended: false },
                { key: "XIU2-http", name: qsTr("HTTP / HTTPS only"), desc: qsTr("Only HTTP and HTTPS trackers"), recommended: false },
                { key: "XIU2-nohttp", name: qsTr("UDP / WSS only"), desc: qsTr("Excludes HTTP and HTTPS trackers"), recommended: false }
            ]
        },
        {
            provider: "newTrackon",
            hint: qsTr("Monitors public trackers and lists the reliable ones"),
            sources: [
                { key: "newtrackon-stable", name: qsTr("Stable"), desc: qsTr("Trackers with at least 95% uptime"), recommended: false }
            ]
        }
    ]

    // 内置逻辑源 key 全集,用于旧配置名归一化判断
    readonly property var builtinKeys: {
        var keys = []
        for (var i = 0; i < sourceGroups.length; ++i) {
            var sources = sourceGroups[i].sources
            for (var j = 0; j < sources.length; ++j) {
                keys.push(sources[j].key)
            }
        }
        return keys
    }

    // 自定义源为用户直接填写的列表 URL
    function isCustomSourceUrl(name) {
        return name.indexOf("http://") === 0 || name.indexOf("https://") === 0
    }

    // 旧版「源×通道」名称(-link/-mirror/-cdn 后缀)归一化为逻辑源名;未知名称原样返回
    function normalizeSourceName(name) {
        if (isCustomSourceUrl(name)) {
            return name
        }
        var suffixes = ["-link", "-mirror", "-cdn"]
        for (var i = 0; i < suffixes.length; ++i) {
            var suffix = suffixes[i]
            if (name.length > suffix.length && name.lastIndexOf(suffix) === name.length - suffix.length) {
                var stripped = name.substring(0, name.length - suffix.length)
                if (builtinKeys.indexOf(stripped) !== -1) {
                    return stripped
                }
            }
        }
        return name
    }

    // 当前选中的源(逻辑源名 + 自定义 URL);读取时归一化并去重
    property var selectedItems: {
        var raw
        // 防御:配置被外部写成空串/非法 JSON 时 JSON.parse 会抛异常
        try {
            raw = JSON.parse(SettingsManager.qTrackerSourceNames || "[]")
        } catch (e) {
            raw = []
        }
        if (!Array.isArray(raw)) {
            raw = []
        }
        var result = []
        for (var i = 0; i < raw.length; ++i) {
            var name = normalizeSourceName(String(raw[i]))
            if (result.indexOf(name) === -1) {
                result.push(name)
            }
        }
        return result
    }

    readonly property var customSources: selectedItems.filter(function(name) {
        return trackServerPage.isCustomSourceUrl(name)
    })

    // 同步进行中标记:用独立属性驱动按钮可用态,避免命令式赋值破坏 enabled 绑定
    property bool syncing: false

    // 勾选/取消一个源并即时提交;赋值新数组以触发绑定更新
    function setSourceSelected(key, on) {
        var items = selectedItems
        var newItems
        if (on) {
            newItems = items.indexOf(key) === -1 ? items.concat(key) : items
        } else {
            newItems = items.filter(function(name) { return name !== key })
        }
        selectedItems = newItems
        SettingsManager.SetTrackerSourceNames(JSON.stringify(newItems))
    }

    // ========== Tracker 源选择区域(按提供方分组) ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Repeater {
            model: trackServerPage.sourceGroups

            ColumnLayout {
                id: groupColumn
                required property var modelData
                required property int index

                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                // 分组标头:提供方名 + 说明
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        text: groupColumn.modelData.provider
                        font.pixelSize: GTheme.fontBody
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                    }

                    Text {
                        Layout.fillWidth: true
                        text: groupColumn.modelData.hint
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                }

                // 源列表:两列布局提升可读性(布局业务值,非尺寸令牌)
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: GTheme.spaceLG
                    rowSpacing: GTheme.spaceSM

                    Repeater {
                        model: groupColumn.modelData.sources

                        RowLayout {
                            id: sourceRow
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: GTheme.spaceSM

                            GCheckBox {
                                id: sourceCheckBox
                                Layout.alignment: Qt.AlignTop
                                checked: trackServerPage.selectedItems.indexOf(sourceRow.modelData.key) !== -1
                                Accessible.name: sourceRow.modelData.name
                                onClicked: {
                                    trackServerPage.setSourceSelected(sourceRow.modelData.key, checked)
                                    // 点击会破坏 checked 绑定,重建以保持与 selectedItems 同步
                                    checked = Qt.binding(function() {
                                        return trackServerPage.selectedItems.indexOf(sourceRow.modelData.key) !== -1
                                    })
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: GTheme.spaceXS

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: GTheme.spaceSM

                                    Text {
                                        text: sourceRow.modelData.name
                                        font.pixelSize: GTheme.fontBody
                                        font.weight: GTheme.weightMedium
                                        color: GTheme.textPrimary
                                    }

                                    // 推荐标记(Element Plus success 色系药丸标签)
                                    Rectangle {
                                        visible: sourceRow.modelData.recommended
                                        implicitWidth: recommendedText.implicitWidth + GTheme.spaceSM * 2
                                        implicitHeight: recommendedText.implicitHeight + GTheme.spaceXS
                                        radius: height / 2
                                        color: GTheme.bgSuccess

                                        Text {
                                            id: recommendedText
                                            anchors.centerIn: parent
                                            text: qsTr("Recommended")
                                            font.pixelSize: GTheme.fontCaption
                                            color: GTheme.successColor
                                        }
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: sourceRow.modelData.desc
                                    font.pixelSize: GTheme.fontCaption
                                    color: GTheme.textSecondary
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                // 组间分隔线(最后一组后不加,由外层 Divider 收尾)
                Divider {
                    Layout.fillWidth: true
                    visible: groupColumn.index < trackServerPage.sourceGroups.length - 1
                    color: GTheme.borderLighter
                }
            }
        }
    }

    // 分隔线(统一 Divider,色 borderLight)
    Divider {
        Layout.fillWidth: true
    }

    // ========== 自定义源区域 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceSM

        Text {
            Layout.fillWidth: true
            text: qsTr("Custom Sources")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightDemiBold
            color: GTheme.textPrimary
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Add your own tracker list URLs (plain text, one tracker per line)")
            font.pixelSize: GTheme.fontCaption
            color: GTheme.textSecondary
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: trackServerPage.customSources

            Rectangle {
                id: customSourceRow
                required property string modelData

                Layout.fillWidth: true
                implicitHeight: customRowLayout.implicitHeight + GTheme.spaceSM * 2
                radius: GTheme.radiusBase
                color: GTheme.fillLighter
                border.color: GTheme.borderLight
                border.width: 1

                RowLayout {
                    id: customRowLayout
                    anchors.fill: parent
                    anchors.leftMargin: GTheme.spaceMD
                    anchors.rightMargin: GTheme.spaceSM
                    spacing: GTheme.spaceSM

                    Text {
                        Layout.fillWidth: true
                        text: customSourceRow.modelData
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textRegular
                        elide: Text.ElideMiddle
                    }

                    GButton {
                        buttonType: "danger"
                        variant: "link"
                        size: "small"
                        text: qsTr("Remove")
                        Accessible.name: qsTr("Remove custom tracker source")
                        onClicked: {
                            trackServerPage.setSourceSelected(customSourceRow.modelData, false)
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            GTextField {
                id: customSourceInput
                Layout.fillWidth: true
                placeholderText: qsTr("https://example.com/trackers.txt")
                Accessible.name: qsTr("Custom tracker source URL")
                onTextEdited: {
                    // 重新编辑时清除错误态
                    if (status !== "normal") {
                        status = "normal"
                    }
                }
                onAccepted: trackServerPage.addCustomSource()
            }

            GButton {
                type: 1
                text: qsTr("Add")
                enabled: customSourceInput.text.trim().length > 0
                Layout.preferredHeight: GTheme.sizeDefault
                Accessible.name: qsTr("Add custom tracker source")
                onClicked: trackServerPage.addCustomSource()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: customSourceInput.status === "error"
            text: qsTr("URL must start with http:// or https://")
            font.pixelSize: GTheme.fontCaption
            color: GTheme.dangerColor
            wrapMode: Text.WordWrap
        }
    }

    // 校验并添加自定义源 URL
    function addCustomSource() {
        var url = customSourceInput.text.trim()
        if (url.length === 0) {
            return
        }
        if (!isCustomSourceUrl(url)) {
            customSourceInput.status = "error"
            return
        }
        customSourceInput.status = "normal"
        setSourceSelected(url, true)
        customSourceInput.text = ""
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
                enabled: !trackServerPage.syncing && trackServerPage.selectedItems.length > 0
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    BrowserManager.SyncTrackersServerlist()
                }
            }

            Text {
                Layout.fillWidth: true
                text: trackServerPage.selectedItems.length > 0
                      ? qsTr("%1 source(s) selected").arg(trackServerPage.selectedItems.length)
                      : qsTr("Select at least one source to sync")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                elide: Text.ElideRight
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
                Accessible.name: qsTr("Automatically update tracker sources")
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

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            Text {
                text: qsTr("Current Tracker List")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                Layout.fillWidth: true
                text: {
                    var list = UtilsToolsManager.serverList
                    if (!list || list.trim() === "") {
                        return ""
                    }
                    var count = list.trim().split("\n").filter(function(line) {
                        return line.trim() !== ""
                    }).length
                    return qsTr("%1 tracker(s)").arg(count)
                }
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                elide: Text.ElideRight
            }
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
                placeholderText: qsTr("No trackers synced yet")
                placeholderTextColor: GTheme.textPlaceholder
                selectByMouse: true
                readOnly: true
                wrapMode: TextArea.Wrap
                Accessible.name: qsTr("Tracker server update result")
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
                        trackServerPage.syncing = true
                    } else if (data.status === "success") {
                        statusText.text = data.message || qsTr("Tracker list updated successfully")
                        statusDetails.text = qsTr("%1 trackers, %2/%3 sources succeeded, took %4ms")
                            .arg(data.tracker_count || 0)
                            .arg(data.successful_sources || 0)
                            .arg((data.successful_sources || 0) + (data.failed_sources || 0))
                            .arg(data.elapsed_ms || 0)
                        trackServerPage.syncing = false

                        // 13 秒后自动隐藏成功消息(业务时间值)
                        hideTimer.restart()
                    } else if (data.status === "error") {
                        statusText.text = data.message || qsTr("Failed to update tracker list")
                        statusDetails.text = data.error || ""
                        trackServerPage.syncing = false

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
