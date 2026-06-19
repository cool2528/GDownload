import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 Aria2 RPC 设置卡片
GCard {
    id: aria2RpcSettingPage
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16  // Element Plus 标准内边距

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
                text: qsTr("Aria2 RPC Settings")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure Aria2 remote RPC listening port")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }


        // RPC 端口配置区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // 子标题
            Text {
                text: qsTr("RPC Listen Port")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            // 端口输入区域
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Port:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GTextField {
                    id: portInput
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    text: SettingsManager.qRpcListenPort.toString()
                    placeholderText: "16888"

                    // 只允许输入数字
                    validator: IntValidator {
                        bottom: 1024
                        top: 65535
                    }
                }

                Text {
                    text: qsTr("(1024-65535)")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        // RPC Secret 配置区域
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: GTheme.borderLight
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            // 子标题
            Text {
                text: qsTr("RPC Secret")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            // Secret 输入区域
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Secret:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GTextField {
                    id: secretInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    text: SettingsManager.qRpcSecret
                    placeholderText: qsTr("Generated automatically on first launch")
                    echoMode: showSecretCheckbox.checked ? TextInput.Normal : TextInput.Password
                }

                // 显示/隐藏密码
                Row {
                    spacing: 6

                    GCheckBox {
                        id: showSecretCheckbox
                        checked: false
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: qsTr("Show")
                        font.pixelSize: 12
                        color: GTheme.textRegular
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                GButton {
                    text: qsTr("Generate Random")
                    type: 3
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 32
                    onClicked: {
                        secretInput.text = SettingsManager.GenerateRpcSecret()
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
                type: 3
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    let randomSecret = SettingsManager.GenerateRpcSecret()
                    portInput.text = "16888"
                    secretInput.text = randomSecret
                    SettingsManager.SetRpcListenPort(16888)
                    SettingsManager.SetRpcSecret(randomSecret)


                    ToastManager.ShowWarning(
                        qsTr("RPC settings reset to default. Please restart the application!"),
                        5000
                    )

                    statusText.text = qsTr("✓ Settings reset to default (Restart required)")
                    statusText.color = GTheme.warningColor
                }
            }

            GButton {
                text: qsTr("Save Settings")
                type: 1  // 主要按钮样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    // 验证端口
                    let portValue = parseInt(portInput.text)
                    // 空输入时 parseInt 返回 NaN，NaN 的所有比较均为 false 会绕过校验，必须显式判断
                    if (isNaN(portValue) || portValue < 1024 || portValue > 65535) {
                        statusText.text = qsTr("✗ Invalid port. Please enter a value between 1024 and 65535.")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("Invalid port number!"), 3000)
                        return
                    }

                    // 验证密钥
                    if (secretInput.text.trim() === "") {
                        statusText.text = qsTr("✗ Secret cannot be empty!")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("Secret cannot be empty!"), 3000)
                        return
                    }

                    // 检查是否有更改
                    let originalPort = SettingsManager.qRpcListenPort
                    let originalSecret = SettingsManager.qRpcSecret
                    let hasChanges = (portValue !== originalPort) || (secretInput.text.trim() !== originalSecret)

                    // 显示 Toast 通知
                    if (hasChanges) {
                        ToastManager.ShowSuccess(
                            qsTr("✓ RPC settings saved successfully! Please restart the application to apply changes."),
                            5000
                        )
                        statusText.text = qsTr("✓ Settings saved: Port=%1, Secret updated (Restart required)").arg(portValue)
                        statusText.color = GTheme.successColor
                        // 保存设置
                        SettingsManager.SetRpcListenPort(portValue)
                        SettingsManager.SetRpcSecret(secretInput.text.trim())
                        UtilsToolsManager.RelaunchAfterExit(3000)
                        Qt.quit()
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
            font.pixelSize: 12
            color: GTheme.textSecondary
            wrapMode: Text.WordWrap
            text: ""
        }

        // 帮助信息区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: helpText.implicitHeight + 50
            color: GTheme.fillLight
            border.color: GTheme.borderLight
            border.width: 1
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Row {
                    spacing: 6

                    Text {
                        text: "ⓘ"
                        font.pixelSize: 16
                        color: GTheme.primaryColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: qsTr("Important Information")
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Text {
                    id: helpText
                    Layout.fillWidth: true
                    font.pixelSize: 12
                    color: GTheme.textRegular
                    wrapMode: Text.WordWrap
                    textFormat: Text.RichText
                    text: qsTr("• The RPC port is used for communication between the application and Aria2 engine<br>" +
                              "• Default port is <b>16888</b><br>" +
                              "• The RPC secret is used for authentication between the application and Aria2 engine<br>" +
                              "• The RPC secret is generated automatically on first launch<br>" +
                              "• It's recommended to use a strong random secret for security<br>" +
                              "• Make sure the port is not used by other applications<br>" +
                              "• <b style='color: %1'>⚠️ You MUST restart the application after changing the port or secret!</b><br>" +
                              "• Choose a port number between 1024 and 65535<br>" +
                              "• Changes are saved immediately but only take effect after restart").arg(GTheme.warningColor)
                }
            }
        }
    }
}
