import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 User-Agent 设置卡片
GCard {
    id: userAgentSettingPage
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16  // Element Plus 标准内边距

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

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 卡片标题和描述
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("User-Agent")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure HTTP/HTTPS User-Agent string for compatibility with different servers")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 预设选择
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Preset User-Agent")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: userAgentPresetComboBox
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                // 创建预设名称列表
                model: {
                    var names = [];
                    for (var i = 0; i < userAgentPresets.length; i++) {
                        names.push(userAgentPresets[i].name);
                    }
                    return names;
                }

                Component.onCompleted: {
                    // 初始化：根据当前配置选择对应预设
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
                            // Custom 选项：保持当前输入框的值
                            customUserAgentField.focus = true;
                        }
                    }
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderBase
        }

        // 自定义输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Custom User-Agent")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            GTextField {
                id: customUserAgentField
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                placeholderText: qsTr("Enter custom User-Agent string...")

                onTextChanged: {
                    // 当用户手动编辑时，自动切换到 "Custom" 预设
                    if (activeFocus) {
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
        }

        // 统一操作按钮区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 16

            Item {
                Layout.fillWidth: true
            }

            GButton {
                text: qsTr("Reset to Default")
                type: 3  // Warning 样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    userAgentPresetComboBox.currentIndex = 0  // Aria2 Default
                    customUserAgentField.text = userAgentPresets[0].value

                    statusText.text = qsTr("Reset to Aria2 default User-Agent (not saved yet)")
                    statusText.color = GTheme.textSecondary
                }
            }

            GButton {
                text: qsTr("Save Settings")
                type: 1  // 主要按钮样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    // 获取输入值
                    let userAgent = customUserAgentField.text.trim()

                    // 验证不能为空
                    if (userAgent === "") {
                        statusText.text = qsTr("✗ User-Agent cannot be empty!")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("User-Agent cannot be empty!"), 3000)
                        return
                    }

                    // 检查是否有更改
                    let originalUserAgent = SettingsManager.qUserAgent

                    if (userAgent !== originalUserAgent) {
                        // 保存设置（会自动通过 RPC 更新 aria2c）
                        SettingsManager.SetAria2UserAgent(userAgent)

                        ToastManager.ShowSuccess(
                            qsTr("✓ User-Agent settings saved and applied successfully!"),
                            3000
                        )

                        // 显示当前预设名称（如果匹配）
                        let presetName = qsTr("Custom")
                        for (let i = 0; i < userAgentPresets.length - 1; i++) {
                            if (userAgentPresets[i].value === userAgent) {
                                presetName = userAgentPresets[i].name
                                break
                            }
                        }

                        statusText.text = qsTr("✓ Settings saved: %1").arg(presetName)
                        statusText.color = GTheme.successColor
                    } else {
                        ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
                        statusText.text = qsTr("Settings unchanged.")
                        statusText.color = GTheme.textSecondary
                    }
                }
            }
        }

        // 状态提示文本
        Text {
            id: statusText
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Select a preset or enter custom User-Agent, then click 'Save Settings' to apply.")
            font.pixelSize: 12
            color: GTheme.textSecondary
            wrapMode: Text.WordWrap
        }

        // 说明提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: tipText.implicitHeight + 24
            color: GTheme.bgInfo
            radius: 4
            border.color: GTheme.borderInfo
            border.width: 1

            Text {
                id: tipText
                anchors.fill: parent
                anchors.margins: 12
                text: qsTr("💡 Tip: Some servers may block downloads from Aria2. Use a browser User-Agent to bypass restrictions.")
                font.pixelSize: 12
                color: GTheme.textInfo
                wrapMode: Text.WordWrap
            }
        }
    }
}
