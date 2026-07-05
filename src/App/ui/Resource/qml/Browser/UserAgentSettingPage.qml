import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 User-Agent 设置卡片(暂存-保存派)
// 配置 HTTP/HTTPS User-Agent:本地暂存预设/自定义输入,Save 时校验非空并比对旧值后提交
// 骨架→SettingCard(单层 spaceLG 内边距,消除原双重 16 边距);
// 预设/自定义子区为"子标题 + 全宽控件"结构(非 label+control+hint 行,保留 ColumnLayout);
// 操作区→SettingFormActions;信息提示框→AlertTip(替换原幽灵令牌 bgInfo/borderInfo/textInfo 裸 Rectangle)
// 颜色/尺寸/间距/字号/圆角/动效一律取自 GTheme 令牌;buttonWidth 为页面级布局常量(spec 2.3)
SettingCard {
    id: userAgentSettingPage
    Layout.fillWidth: true

    title: qsTr("User-Agent")
    description: qsTr("Configure HTTP/HTTPS User-Agent string for compatibility with different servers")

    // 页面级布局常量(非设计令牌,spec 2.3 约定:按钮宽度)
    readonly property int buttonWidth: 120

    // User-Agent 预设列表
    property var userAgentPresets: [
        {
            name: qsTr("Aria2 Default"),
            value: "aria2/1.36.0"
        },
        {
            name: qsTr("Chrome 120 (Windows)"),
            value: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
        },
        {
            name: qsTr("Firefox 121 (Windows)"),
            value: "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:121.0) Gecko/20100101 Firefox/121.0"
        },
        {
            name: qsTr("Safari 17 (macOS)"),
            value: "Mozilla/5.0 (Macintosh; Intel Mac OS X 14_1) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Safari/605.1.15"
        },
        {
            name: qsTr("Edge 120 (Windows)"),
            value: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0"
        },
        {
            name: qsTr("Chrome 120 (Android)"),
            value: "Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
        },
        {
            name: qsTr("Safari (iPhone)"),
            value: "Mozilla/5.0 (iPhone; CPU iPhone OS 17_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Mobile/15E148 Safari/604.1"
        },
        {
            name: qsTr("Wget 1.21"),
            value: "Wget/1.21.3"
        },
        {
            name: qsTr("cURL 8.5"),
            value: "curl/8.5.0"
        },
        {
            name: qsTr("Custom"),
            value: ""
        }
    ]

    // ========== 预设选择 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Preset User-Agent")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        GComBoBox {
            id: userAgentPresetComboBox
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault

            // 创建预设名称列表
            model: {
                var names = [];
                for (var i = 0; i < userAgentPresets.length; i++) {
                    names.push(userAgentPresets[i].name);
                }
                return names;
            }

            Component.onCompleted: {
                // 初始化:根据当前配置选择对应预设
                var currentUA = SettingsManager.qUserAgent;
                var foundIndex = -1;

                for (var i = 0; i < userAgentPresets.length - 1; i++) {  // 不包括 "Custom"
                    if (userAgentPresets[i].value === currentUA) {
                        foundIndex = i;
                        break;
                    }
                }

                if (foundIndex >= 0) {
                    currentIndex = foundIndex;
                    customUserAgentField.text = userAgentPresets[foundIndex].value;
                } else {
                    currentIndex = userAgentPresets.length - 1;  // Custom
                    customUserAgentField.text = currentUA;
                }
            }

            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < userAgentPresets.length) {
                    var selectedUA = userAgentPresets[currentIndex].value;
                    if (selectedUA !== "") {
                        customUserAgentField.text = selectedUA;
                    } else {
                        // Custom 选项:保持当前输入框的值
                        customUserAgentField.focus = true;
                    }
                }
            }
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: GTheme.borderLight
    }

    // ========== 自定义输入 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Custom User-Agent")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        GTextField {
            id: customUserAgentField
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault
            placeholderText: qsTr("Enter custom User-Agent string...")

            onEditingFinished: {
                // 编辑完成后才做预设匹配,避免逐字输入时预设回填覆盖用户文本(X4)
                var isPreset = false;
                for (var i = 0; i < userAgentPresets.length - 1; i++) {
                    if (userAgentPresets[i].value === text) {
                        isPreset = true;
                        userAgentPresetComboBox.currentIndex = i;
                        break;
                    }
                }
                if (!isPreset && userAgentPresetComboBox.currentIndex !== userAgentPresets.length - 1) {
                    userAgentPresetComboBox.currentIndex = userAgentPresets.length - 1;  // Custom
                }
            }
        }
    }

    // ========== 操作区:Reset + Save + 状态提示(暂存-保存派,spec 2.3)==========
    SettingFormActions {
        id: formActions
        Layout.fillWidth: true
        Layout.topMargin: GTheme.spaceLG
        buttonWidth: userAgentSettingPage.buttonWidth
        defaultStatusText: qsTr("Select a preset or enter custom User-Agent, then click 'Save Settings' to apply.")

        // 有未保存改动时才启用 Save
        hasChanges: customUserAgentField.text.trim() !== SettingsManager.qUserAgent

        onReset: {
            // 重置为默认预设 Aria2 Default(仅重置输入,未保存)
            userAgentPresetComboBox.currentIndex = 0  // Aria2 Default
            customUserAgentField.text = userAgentPresets[0].value

            formActions.statusText = qsTr("Reset to Aria2 default User-Agent (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 获取输入值
            let userAgent = customUserAgentField.text.trim()

            // 验证不能为空
            if (userAgent === "") {
                formActions.statusText = qsTr("✗ User-Agent cannot be empty!")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("User-Agent cannot be empty!"), 3000)
                return
            }

            // 检查是否有更改
            let originalUserAgent = SettingsManager.qUserAgent

            if (userAgent !== originalUserAgent) {
                // 保存设置(会自动通过 RPC 更新 aria2c)
                SettingsManager.SetAria2UserAgent(userAgent)

                ToastManager.ShowSuccess(
                    qsTr("✓ User-Agent settings saved and applied successfully!"),
                    3000
                )

                // 显示当前预设名称(如果匹配)
                let presetName = qsTr("Custom")
                for (let i = 0; i < userAgentPresets.length - 1; i++) {
                    if (userAgentPresets[i].value === userAgent) {
                        presetName = userAgentPresets[i].name
                        break
                    }
                }

                formActions.statusText = qsTr("✓ Settings saved: %1").arg(presetName)
                formActions.statusColor = GTheme.successColor
            } else {
                ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
                formActions.statusText = qsTr("Settings unchanged.")
                formActions.statusColor = GTheme.textSecondary
            }
        }
    }

    // ========== 信息提示框:UA 绕过说明(告警框→AlertTip,替换原幽灵令牌裸 Rectangle,spec 2.4)==========
    AlertTip {
        Layout.fillWidth: true
        severity: "info"
        text: qsTr("Tip: Some servers may block downloads from Aria2. Use a browser User-Agent to bypass restrictions.")
    }
}
