import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格连接和性能参数设置卡片(暂存-保存派)
// 并行下载/单服务器连接/分片数/最小分片大小四项:本地暂存输入,Save 时比对旧值后批量提交
// 骨架→SettingCard;设置项行→SettingRow;操作区→SettingFormActions;分隔线→Divider
// 颜色/尺寸/间距/字号/圆角/动效全走 GTheme 令牌;inputWidth 为页面级布局常量,
// sizes/默认值/from..to 为 aria2 业务值(非设计令牌)
SettingCard {
    id: root

    // 页面级布局常量(非设计令牌,spec 2.2 约定:控件宽度)
    readonly property int inputWidth: 150

    // 最小分片大小下拉取值(业务值,单位 MB,与下拉 model 一一对应)
    readonly property var sizes: [1, 5, 10, 20, 50, 100]

    // 父容器(AdvancedSettingPage)已设 Layout.fillWidth/margins;此处保留 fillWidth 以兼容独立使用
    Layout.fillWidth: true

    title: qsTr("Connection & Performance")
    description: qsTr("Configure connection parameters and download performance")

    // ========== 并行下载数 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Max Concurrent Downloads")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Count:")
            hint: qsTr("Number of downloads running at the same time")
            control: GSpinBox {
                id: maxConcurrentDownloadsInput
                // 测试可驱动性钩子:集成测试 findChild 定位此 SpinBox 以触发 hasChanges
                objectName: "maxConcurrentDownloadsInput"
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                // aria2 配置范围(业务值,非设计令牌)
                from: 1
                to: 64
                value: SettingsManager.qMaxConcurrentDownloads
            }
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 单服务器连接数 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Max Connections Per Server")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Count:")
            hint: qsTr("More connections = faster speed, but may be blocked by servers")
            control: GSpinBox {
                id: maxConnectionPerServerInput
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 1
                to: 64
                value: SettingsManager.qMaxConnectionPerServer
            }
        }
    }

    // 分隔线
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 分片数 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("File Segments (Split)")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Count:")
            hint: qsTr("Number of segments to split the file for downloading")
            control: GSpinBox {
                id: splitInput
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 1
                to: 64
                value: SettingsManager.qSplit
            }
        }
    }

    // 分隔线
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 最小分割大小 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Min Split Size")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Size:")
            hint: qsTr("Don't split files smaller than this size")
            control: GComBoBox {
                id: minSplitSizeCombo
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                model: ["1M", "5M", "10M", "20M", "50M", "100M"]

                Component.onCompleted: {
                    // 根据已保存的最小分片大小匹配下拉索引,未匹配时回退 20M(索引 3)
                    let currentSize = SettingsManager.qMinSplitSize
                    let index = root.sizes.indexOf(currentSize)
                    currentIndex = index >= 0 ? index : 3
                }
            }
        }
    }

    // ========== 操作区:Reset + Save + 状态提示(暂存-保存派,spec 2.3)==========
    SettingFormActions {
        id: formActions
        Layout.fillWidth: true
        Layout.topMargin: GTheme.spaceLG

        // 有任一输入偏离已保存值时才启用 Save
        hasChanges: maxConcurrentDownloadsInput.value !== SettingsManager.qMaxConcurrentDownloads ||
                    maxConnectionPerServerInput.value !== SettingsManager.qMaxConnectionPerServer ||
                    splitInput.value !== SettingsManager.qSplit ||
                    root.sizes[minSplitSizeCombo.currentIndex] !== SettingsManager.qMinSplitSize

        onReset: {
            // 重置为 aria2 默认值(业务值,未保存)
            maxConcurrentDownloadsInput.value = 5
            maxConnectionPerServerInput.value = 16
            splitInput.value = 16
            minSplitSizeCombo.currentIndex = 3  // 20M

            formActions.statusText = qsTr("Input fields reset to default values (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 解析输入值(分片大小按下拉索引映射回 MB 数值)
            let concurrentDownloads = maxConcurrentDownloadsInput.value
            let connectionPerServer = maxConnectionPerServerInput.value
            let split = splitInput.value
            let minSplitSize = root.sizes[minSplitSizeCombo.currentIndex]

            // 批量应用:值有变才调 setter,并拼接状态摘要(由 SettingFormActions.applySettings 统一收口)
            formActions.applySettings([
                { val: concurrentDownloads, old: SettingsManager.qMaxConcurrentDownloads,
                  setter: function (v) { SettingsManager.SetAria2MaxConcurrentDownloads(v) },
                  label: qsTr("Concurrent") },
                { val: connectionPerServer, old: SettingsManager.qMaxConnectionPerServer,
                  setter: function (v) { SettingsManager.SetAria2MaxConnectionPerServer(v) },
                  label: qsTr("Connection") },
                { val: split, old: SettingsManager.qSplit,
                  setter: function (v) { SettingsManager.SetAria2Split(v) },
                  label: qsTr("Split") },
                { val: minSplitSize, old: SettingsManager.qMinSplitSize,
                  setter: function (v) { SettingsManager.SetAria2MinSplitSize(v) },
                  label: qsTr("MinSize") }
            ], qsTr("✓ Connection & Performance settings saved and applied successfully!"))
        }
    }
}
