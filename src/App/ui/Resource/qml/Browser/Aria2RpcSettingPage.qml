import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Aria2 RPC 设置卡片:暂存-保存派,配置 Aria2 远程 RPC 监听端口与密钥
// 骨架走 SettingCard;端口行走 SettingRow;操作区走 SettingFormActions;信息提示框走 AlertTip
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌;labelWidth/inputWidth/buttonWidth 为页面级布局常量
SettingCard {
    id: aria2RpcSettingPage
    Layout.fillWidth: true

    title: qsTr("Aria2 RPC Settings")
    description: qsTr("Configure Aria2 remote RPC listening port")

    // 页面级布局常量(非设计令牌,spec 2.2/2.3 约定:label/input/button 宽度)
    readonly property int labelWidth: 60
    readonly property int inputWidth: 150
    readonly property int buttonWidth: 120

    // RPC 端口配置区域
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        // 子标题
        Text {
            text: qsTr("RPC Listen Port")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        // 端口输入行
        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Port:")
            labelWidth: aria2RpcSettingPage.labelWidth
            control: GTextField {
                id: portInput
                Layout.preferredWidth: aria2RpcSettingPage.inputWidth
                text: SettingsManager.qRpcListenPort.toString()
                placeholderText: "16888"

                // 只允许输入数字(业务值:端口上下界)
                validator: IntValidator {
                    bottom: 1024
                    top: 65535
                }
            }
            hint: qsTr("(1024-65535)")
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
    }

    // RPC Secret 配置区域
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        // 子标题
        Text {
            text: qsTr("RPC Secret")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        // Secret 输入行:复合控件(输入框 + 显示切换 + 生成按钮)
        // 因控件为复合结构且密钥为 32 位需 fillWidth 展开容纳,保留令牌化 RowLayout 而非 SettingRow
        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Text {
                text: qsTr("Secret:")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightRegular
                color: GTheme.textRegular
                Layout.preferredWidth: aria2RpcSettingPage.labelWidth
                Layout.alignment: Qt.AlignVCenter
                verticalAlignment: Text.AlignVCenter
            }

            GTextField {
                id: secretInput
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qRpcSecret
                placeholderText: qsTr("Generated automatically on first launch")
                echoMode: showSecretCheckbox.checked ? TextInput.Normal : TextInput.Password
            }

            // 显示/隐藏密码
            Row {
                spacing: GTheme.spaceSM
                Layout.alignment: Qt.AlignVCenter

                GCheckBox {
                    id: showSecretCheckbox
                    checked: false
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: qsTr("Show")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            GButton {
                text: qsTr("Generate Random")
                type: 3  // 次要按钮(default 样式)
                Layout.preferredWidth: aria2RpcSettingPage.buttonWidth
                onClicked: {
                    secretInput.text = SettingsManager.GenerateRpcSecret()
                }
            }
        }
    }

    // 操作区:暂存-保存派 Reset/Save + 状态提示(spec 2.3)
    SettingFormActions {
        id: formActions
        Layout.fillWidth: true
        Layout.topMargin: GTheme.spaceLG
        buttonWidth: aria2RpcSettingPage.buttonWidth
        defaultStatusText: ""
        hasChanges: (parseInt(portInput.text) !== SettingsManager.qRpcListenPort) ||
                    (secretInput.text.trim() !== SettingsManager.qRpcSecret)

        onReset: {
            let randomSecret = SettingsManager.GenerateRpcSecret()
            portInput.text = "16888"
            secretInput.text = randomSecret
            SettingsManager.SetRpcListenPort(16888)
            SettingsManager.SetRpcSecret(randomSecret)

            ToastManager.ShowWarning(
                qsTr("RPC settings reset to default. Please restart the application!"),
                5000
            )

            formActions.statusText = qsTr("✓ Settings reset to default (Restart required)")
            formActions.statusColor = GTheme.warningColor
        }

        onSave: {
            // 验证端口
            let portValue = parseInt(portInput.text)
            // 空输入时 parseInt 返回 NaN,NaN 的所有比较均为 false 会绕过校验,必须显式判断
            if (isNaN(portValue) || portValue < 1024 || portValue > 65535) {
                formActions.statusText = qsTr("✗ Invalid port. Please enter a value between 1024 and 65535.")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("Invalid port number!"), 3000)
                return
            }

            // 验证密钥
            if (secretInput.text.trim() === "") {
                formActions.statusText = qsTr("✗ Secret cannot be empty!")
                formActions.statusColor = GTheme.dangerColor
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
                formActions.statusText = qsTr("✓ Settings saved: Port=%1, Secret updated (Restart required)").arg(portValue)
                formActions.statusColor = GTheme.successColor
                // 保存设置
                SettingsManager.SetRpcListenPort(portValue)
                SettingsManager.SetRpcSecret(secretInput.text.trim())
                UtilsToolsManager.RelaunchAfterExit(3000)
                Qt.quit()
            } else {
                ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
                formActions.statusText = qsTr("Settings unchanged.")
                formActions.statusColor = GTheme.textSecondary
            }
        }
    }

    // 重要信息提示框:端口/密钥使用说明与重启要求(告警框→AlertTip,spec 2.4)
    AlertTip {
        Layout.fillWidth: true
        severity: "info"
        text: qsTr("• The RPC port is used for communication between the application and Aria2 engine\n" +
                   "• Default port is 16888\n" +
                   "• The RPC secret is used for authentication between the application and Aria2 engine\n" +
                   "• The RPC secret is generated automatically on first launch\n" +
                   "• It's recommended to use a strong random secret for security\n" +
                   "• Make sure the port is not used by other applications\n" +
                   "• ⚠️ You MUST restart the application after changing the port or secret!\n" +
                   "• Choose a port number between 1024 and 65535\n" +
                   "• Changes are saved immediately but only take effect after restart")
    }
}
