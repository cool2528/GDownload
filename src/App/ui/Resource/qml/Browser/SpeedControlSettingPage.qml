import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格速度控制设置卡片(暂存-保存派)
// 下载/上传/低速断开阈值三项限速:本地暂存输入,Save 时校验范围并比对旧值后批量提交
// 骨架→SettingCard;设置项行→SettingRow;操作区→SettingFormActions;尺寸/间距/字号/圆角/动效全走 GTheme 令牌
SettingCard {
    id: root

    // 页面级布局常量(非设计令牌):输入框宽度
    readonly property int inputWidth: 150

    // 父容器(AdvancedSettingPage)已设 Layout.fillWidth/margins;此处保留 fillWidth 以兼容独立使用
    Layout.fillWidth: true

    title: qsTr("Speed Control")
    description: qsTr("Configure download and upload speed limits")

    // ========== 全局下载限速 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Global Download Limit")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Speed:")
            hint: qsTr("KB/s (0 = unlimited)")
            control: GTextField {
                id: maxOverallDownloadInput
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qMaxOverallDownloadLimit.toString()
                placeholderText: "0"

                // 只允许输入数字
                validator: IntValidator {
                    bottom: 0
                    top: 102400  // 100 MB/s(业务值,非设计令牌)
                }
            }
        }
    }

    // 分隔线
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 全局上传限速 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Global Upload Limit")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Speed:")
            hint: qsTr("KB/s (0 = unlimited, for BitTorrent seeding)")
            control: GTextField {
                id: maxOverallUploadInput
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qMaxOverallUploadLimit.toString()
                placeholderText: "0"

                // 只允许输入数字
                validator: IntValidator {
                    bottom: 0
                    top: 10240  // 10 MB/s(业务值,非设计令牌)
                }
            }
        }
    }

    // 分隔线
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 低速断开阈值 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Lowest Speed Limit")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Speed:")
            hint: qsTr("KB/s (0 = disabled, disconnect if speed is lower for 60s)")
            control: GTextField {
                id: lowestSpeedInput
                Layout.preferredWidth: root.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qLowestSpeedLimit.toString()
                placeholderText: "0"

                // 只允许输入数字
                validator: IntValidator {
                    bottom: 0
                    top: 1024  // 1 MB/s(业务值,非设计令牌)
                }
            }
        }
    }

    // ========== 操作区:Reset + Save + 状态提示 ==========
    SettingFormActions {
        id: formActions

        // 有任一输入偏离已保存值时才启用 Save
        hasChanges: (parseInt(maxOverallDownloadInput.text) || 0) !== SettingsManager.qMaxOverallDownloadLimit ||
                    (parseInt(maxOverallUploadInput.text) || 0) !== SettingsManager.qMaxOverallUploadLimit ||
                    (parseInt(lowestSpeedInput.text) || 0) !== SettingsManager.qLowestSpeedLimit

        onReset: {
            // 重置为默认值 0(未保存)
            maxOverallDownloadInput.text = "0"
            maxOverallUploadInput.text = "0"
            lowestSpeedInput.text = "0"
            formActions.statusText = qsTr("Input fields reset to default values (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 解析输入值
            let downloadLimit = parseInt(maxOverallDownloadInput.text) || 0
            let uploadLimit = parseInt(maxOverallUploadInput.text) || 0
            let lowestSpeed = parseInt(lowestSpeedInput.text) || 0

            // 范围校验(业务边界值,非设计令牌)
            if (downloadLimit < 0 || downloadLimit > 102400) {
                formActions.statusText = qsTr("✗ Download limit must be between 0 and 102400 KB/s")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("Invalid download limit!"), 3000)
                return
            }
            if (uploadLimit < 0 || uploadLimit > 10240) {
                formActions.statusText = qsTr("✗ Upload limit must be between 0 and 10240 KB/s")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("Invalid upload limit!"), 3000)
                return
            }
            if (lowestSpeed < 0 || lowestSpeed > 1024) {
                formActions.statusText = qsTr("✗ Lowest speed must be between 0 and 1024 KB/s")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("Invalid lowest speed!"), 3000)
                return
            }

            // 批量应用:值有变才调 setter,并拼接状态摘要(由 SettingFormActions.applySettings 统一收口)
            formActions.applySettings([
                { val: downloadLimit, old: SettingsManager.qMaxOverallDownloadLimit,
                  setter: function (v) { SettingsManager.SetAria2MaxOverallDownloadLimit(v) },
                  label: qsTr("Download") },
                { val: uploadLimit, old: SettingsManager.qMaxOverallUploadLimit,
                  setter: function (v) { SettingsManager.SetAria2MaxOverallUploadLimit(v) },
                  label: qsTr("Upload") },
                { val: lowestSpeed, old: SettingsManager.qLowestSpeedLimit,
                  setter: function (v) { SettingsManager.SetAria2LowestSpeedLimit(v) },
                  label: qsTr("LowestSpeed") }
            ], qsTr("✓ Speed control settings saved and applied successfully!"))
        }
    }
}
