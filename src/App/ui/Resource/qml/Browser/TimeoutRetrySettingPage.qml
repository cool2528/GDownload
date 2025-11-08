import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格超时和重试设置卡片
GCard {
    id: timeoutRetrySettingPage
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
                text: qsTr("Timeout & Retry Settings")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure connection timeout and retry behavior for unstable networks")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 超时设置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Timeout")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                GSpinBox {
                    id: timeoutSpinBox
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    from: 1
                    to: 600
                    value: SettingsManager.qTimeout
                }

                Text {
                    text: qsTr("seconds")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("HTTP/FTP connection timeout after establishment")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // 连接超时设置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Connect Timeout")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                GSpinBox {
                    id: connectTimeoutSpinBox
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    from: 1
                    to: 300
                    value: SettingsManager.qConnectTimeout
                }

                Text {
                    text: qsTr("seconds")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Timeout for establishing initial connection")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderBase
        }

        // 重试设置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Max Retry Attempts")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                GSpinBox {
                    id: maxTriesSpinBox
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    from: 0
                    to: 999
                    value: SettingsManager.qMaxTries
                }

                Text {
                    text: qsTr("times")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Number of retry attempts (0 = unlimited)")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // 重试等待时间
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Retry Wait Time")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                GSpinBox {
                    id: retryWaitSpinBox
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    from: 0
                    to: 600
                    value: SettingsManager.qRetryWait
                }

                Text {
                    text: qsTr("seconds")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Wait time between retries (0 = disabled, only retry on HTTP 503)")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
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
                    timeoutSpinBox.value = 60
                    connectTimeoutSpinBox.value = 60
                    maxTriesSpinBox.value = 5
                    retryWaitSpinBox.value = 0

                    statusText.text = qsTr("Input fields reset to default values (not saved yet)")
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
                    let timeout = timeoutSpinBox.value
                    let connectTimeout = connectTimeoutSpinBox.value
                    let maxTries = maxTriesSpinBox.value
                    let retryWait = retryWaitSpinBox.value

                    // 检查是否有更改
                    let originalTimeout = SettingsManager.qTimeout
                    let originalConnectTimeout = SettingsManager.qConnectTimeout
                    let originalMaxTries = SettingsManager.qMaxTries
                    let originalRetryWait = SettingsManager.qRetryWait

                    let hasChanges = (timeout !== originalTimeout) ||
                                   (connectTimeout !== originalConnectTimeout) ||
                                   (maxTries !== originalMaxTries) ||
                                   (retryWait !== originalRetryWait)

                    if (hasChanges) {
                        // 保存设置（会自动通过 RPC 更新 aria2c）
                        SettingsManager.SetAria2Timeout(timeout)
                        SettingsManager.SetAria2ConnectTimeout(connectTimeout)
                        SettingsManager.SetAria2MaxTries(maxTries)
                        SettingsManager.SetAria2RetryWait(retryWait)

                        ToastManager.ShowSuccess(
                            qsTr("✓ Timeout and retry settings saved and applied successfully!"),
                            3000
                        )

                        statusText.text = qsTr("✓ Settings saved: Timeout=%1s, ConnectTimeout=%2s, MaxTries=%3, RetryWait=%4s")
                            .arg(timeout).arg(connectTimeout).arg(maxTries).arg(retryWait)
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
            text: qsTr("Modify the values above and click 'Save Settings' to apply.")
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
                text: qsTr("💡 Tip: Increase timeout and retry values for unstable network connections")
                font.pixelSize: 12
                color: GTheme.textInfo
                wrapMode: Text.WordWrap
            }
        }
    }
}
