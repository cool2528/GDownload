import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk

// 插件市场页面：浏览/安装/更新/卸载/启用禁用 网盘解析插件
// 数据来自 PluginMarketManager 单例（C++）；所有颜色/间距/圆角取自 GTheme 令牌
Item {
    id: root

    // 当前筛选：0=全部 1=已安装 2=可更新
    property int filterIndex: 0

    // 配置修订号：保存/清除插件配置后自增，强制重算依赖 hasSchema 的绑定
    property int configRevision: 0

    // 状态枚举（与 C++ InstallState / Busy 对齐）
    readonly property int stAvailable: 0
    readonly property int stInstalled: 1
    readonly property int stUpdate: 2
    readonly property int stBusy: 3

    Component.onCompleted: PluginMarketManager.refresh()

    Connections {
        target: PluginMarketManager
        function onRefreshFinished(success, message) {
            if (success)
                ToastManager.ShowSuccess(qsTr("Plugin list refreshed"))
            else if (message.length > 0)
                ToastManager.ShowError(qsTr("Failed to load plugin market: %1").arg(message))
        }
        function onOperationFinished(name, success, message) {
            if (success)
                ToastManager.ShowSuccess(qsTr("%1 succeeded").arg(name))
            else if (message.length > 0)
                ToastManager.ShowError(qsTr("%1 failed: %2").arg(name).arg(message))
        }
    }

    Connections {
        target: PluginConfigManager
        // 保存/清除配置后强制重算 hasSettings/configured 绑定
        function onConfigChanged(name) {
            root.configRevision++
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: GTheme.spaceLG

        // ---- 顶部工具栏：搜索 + 源 + 刷新 ----
        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            // 筛选标签
            Row {
                spacing: GTheme.spaceXS
                Repeater {
                    model: [qsTr("All"), qsTr("Installed"), qsTr("Updates")]
                    GButton {
                        variant: "chip"
                        checkable: true
                        checked: root.filterIndex === index
                        text: modelData
                        onClicked: root.filterIndex = index
                    }
                }
            }

            Item { Layout.fillWidth: true }

            GTextField {
                id: searchField
                Layout.preferredWidth: 200
                placeholderText: qsTr("Search plugins…")
            }

            // 刷新按钮：刷新时图标显式旋转（不依赖 BusyIndicator 样式，保证动起来）
            GButton {
                id: refreshBtn
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                iconName: "refresh"
                variant: "default"
                Accessible.name: qsTr("Refresh")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Refresh plugin list")
                onClicked: PluginMarketManager.refresh()

                RotationAnimation on rotation {
                    id: spinAnim
                    running: PluginMarketManager.busy
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 900
                    onRunningChanged: if (!running) refreshBtn.rotation = 0
                }
            }
        }

        // ---- 插件卡片网格 ----
        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 380)))
            cellHeight: 240
            model: PluginMarketManager.model
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                width: grid.cellWidth
                height: grid.cellHeight

                // 按筛选隐藏不匹配项
                readonly property bool matchFilter: {
                    if (root.filterIndex === 1)
                        return model.state === root.stInstalled || model.state === root.stUpdate
                               || (model.installedVersion.length > 0)
                    if (root.filterIndex === 2)
                        return model.state === root.stUpdate
                    return true
                }
                visible: matchFilter
                enabled: matchFilter

                PluginMarketCard {
                    anchors.fill: parent
                    anchors.margins: GTheme.spaceSM
                    pluginName: model.name
                    displayName: model.displayName
                    description: model.description
                    author: model.author
                    verified: model.verified
                    latestVersion: model.latestVersion
                    installedVersion: model.installedVersion
                    state: model.state
                    enabledState: model.enabled
                    progress: model.progress
                    stage: model.stage
                    tags: model.tags
                    hasSettings: (root.configRevision, PluginConfigManager.hasSchema(model.name))

                    onInstall: PluginMarketManager.install(model.name)
                    onUpdatePlugin: PluginMarketManager.install(model.name)
                    onUninstall: PluginMarketManager.uninstall(model.name)
                    onToggleEnabled: PluginMarketManager.setEnabled(model.name, on)
                    onOpenSettings: marketSettingsDialog.openFor(model.name)
                }
            }
        }
    }

    // 空态
    ColumnLayout {
        anchors.centerIn: parent
        spacing: GTheme.spaceMD
        visible: grid.count === 0 && !PluginMarketManager.busy
        AuroraIcon {
            Layout.alignment: Qt.AlignHCenter
            name: "repository"
            iconSize: GTheme.space3XL
            color: GTheme.textPlaceholder
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("No plugins available")
            font.pixelSize: GTheme.fontSubtitle
            color: GTheme.textSecondary
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Check your network connection and refresh")
            font.pixelSize: GTheme.fontBody
            color: GTheme.textPlaceholder
        }
    }

    // 加载指示
    BusyIndicator {
        anchors.centerIn: parent
        running: PluginMarketManager.busy && grid.count === 0
        visible: running
    }

    PluginSettingsDialog {
        id: marketSettingsDialog
        parent: Overlay.overlay
    }
}
