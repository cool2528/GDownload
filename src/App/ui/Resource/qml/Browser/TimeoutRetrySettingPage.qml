import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格超时和重试设置卡片(暂存-保存派)
// 超时/连接超时/最大重试/重试等待四项:本地暂存,Save 时比对旧值后批量提交
// 骨架→SettingCard;设置项行→SettingRow;操作区→SettingFormActions;告警框→AlertTip
// 尺寸/间距/字号/圆角/动效全走 GTheme 令牌,零魔法数字(业务值除外)
SettingCard {
    id: root

    // 页面级布局常量(非设计令牌):数值输入框宽度
    readonly property int inputWidth: 120
    // 页面级布局常量(非设计令牌):设置项标签列宽,按最长标签 "Max Retry Attempts" 适配(spec 2.2)
    readonly property int labelWidth: 150

    // 父容器(AdvancedSettingPage)已设 Layout.fillWidth/margins;此处保留 fillWidth 以兼容独立使用
    Layout.fillWidth: true

    title: qsTr("Timeout & Retry Settings")
    description: qsTr("Configure connection timeout and retry behavior for unstable networks")

    // ========== 超时设置 ==========
    SettingRow {
        Layout.fillWidth: true
        labelWidth: root.labelWidth
        label: qsTr("Timeout")
        hint: qsTr("seconds") + " — " + qsTr("HTTP/FTP connection timeout after establishment")
        control: GSpinBox {
            id: timeoutSpinBox
            Layout.preferredWidth: root.inputWidth
            Layout.preferredHeight: GTheme.sizeDefault
            from: 1
            to: 600
            value: SettingsManager.qTimeout
            Accessible.name: qsTr("Connection timeout")
        }
    }

    // ========== 连接超时设置 ==========
    SettingRow {
        Layout.fillWidth: true
        labelWidth: root.labelWidth
        label: qsTr("Connect Timeout")
        hint: qsTr("seconds") + " — " + qsTr("Timeout for establishing initial connection")
        control: GSpinBox {
            id: connectTimeoutSpinBox
            Layout.preferredWidth: root.inputWidth
            Layout.preferredHeight: GTheme.sizeDefault
            from: 1
            to: 300
            value: SettingsManager.qConnectTimeout
            Accessible.name: qsTr("Initial connection timeout")
        }
    }

    // 分隔线(超时组 / 重试组)
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 最大重试次数 ==========
    SettingRow {
        Layout.fillWidth: true
        labelWidth: root.labelWidth
        label: qsTr("Max Retry Attempts")
        hint: qsTr("times") + " — " + qsTr("Number of retry attempts (0 = unlimited)")
        control: GSpinBox {
            id: maxTriesSpinBox
            Layout.preferredWidth: root.inputWidth
            Layout.preferredHeight: GTheme.sizeDefault
            from: 0
            to: 999
            value: SettingsManager.qMaxTries
            Accessible.name: qsTr("Maximum retry attempts")
        }
    }

    // ========== 重试等待时间 ==========
    SettingRow {
        Layout.fillWidth: true
        labelWidth: root.labelWidth
        label: qsTr("Retry Wait Time")
        hint: qsTr("seconds") + " — " + qsTr("Wait time between retries (0 = disabled, only retry on HTTP 503)")
        control: GSpinBox {
            id: retryWaitSpinBox
            Layout.preferredWidth: root.inputWidth
            Layout.preferredHeight: GTheme.sizeDefault
            from: 0
            to: 600
            value: SettingsManager.qRetryWait
            Accessible.name: qsTr("Retry wait time")
        }
    }

    // ========== 操作区:Reset + Save + 状态提示 ==========
    SettingFormActions {
        id: formActions

        // 有任一输入偏离已保存值时才启用 Save
        hasChanges: timeoutSpinBox.value !== SettingsManager.qTimeout ||
                    connectTimeoutSpinBox.value !== SettingsManager.qConnectTimeout ||
                    maxTriesSpinBox.value !== SettingsManager.qMaxTries ||
                    retryWaitSpinBox.value !== SettingsManager.qRetryWait

        onReset: {
            // 重置为默认值(业务默认值,未保存)
            timeoutSpinBox.value = 60
            connectTimeoutSpinBox.value = 60
            maxTriesSpinBox.value = 5
            retryWaitSpinBox.value = 0
            formActions.statusText = qsTr("Input fields reset to default values (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 批量应用:值有变才调 setter,并拼接状态摘要(由 SettingFormActions.applySettings 统一收口)
            formActions.applySettings([
                { val: timeoutSpinBox.value, old: SettingsManager.qTimeout,
                  setter: function (v) { SettingsManager.SetAria2Timeout(v) },
                  label: qsTr("Timeout") },
                { val: connectTimeoutSpinBox.value, old: SettingsManager.qConnectTimeout,
                  setter: function (v) { SettingsManager.SetAria2ConnectTimeout(v) },
                  label: qsTr("ConnectTimeout") },
                { val: maxTriesSpinBox.value, old: SettingsManager.qMaxTries,
                  setter: function (v) { SettingsManager.SetAria2MaxTries(v) },
                  label: qsTr("MaxTries") },
                { val: retryWaitSpinBox.value, old: SettingsManager.qRetryWait,
                  setter: function (v) { SettingsManager.SetAria2RetryWait(v) },
                  label: qsTr("RetryWait") }
            ], qsTr("Timeout and retry settings saved and applied successfully!"))
        }
    }

    // ========== 说明提示(告警框,替代原裸 Rectangle + 幽灵令牌 bgInfo/borderInfo/textInfo)==========
    AlertTip {
        Layout.fillWidth: true
        severity: "info"
        text: qsTr("Tip: Increase timeout and retry values for unstable network connections")
    }
}
