import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格速度控制设置卡片
GCard {
    id: speedControlSettingPage
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
                text: qsTr("Speed Control")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure download and upload speed limits")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 全局下载限速
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Global Download Limit")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Speed:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GTextField {
                    id: maxOverallDownloadInput
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    text: SettingsManager.qMaxOverallDownloadLimit.toString()
                    placeholderText: "0"

                    // 只允许输入数字
                    validator: IntValidator {
                        bottom: 0
                        top: 102400  // 100 MB/s
                    }
                }

                Text {
                    text: qsTr("KB/s (0 = unlimited)")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        // 全局上传限速
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Global Upload Limit")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Speed:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GTextField {
                    id: maxOverallUploadInput
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    text: SettingsManager.qMaxOverallUploadLimit.toString()
                    placeholderText: "0"

                    // 只允许输入数字
                    validator: IntValidator {
                        bottom: 0
                        top: 10240  // 10 MB/s
                    }
                }

                Text {
                    text: qsTr("KB/s (0 = unlimited, for BitTorrent seeding)")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        // 低速断开阈值
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Lowest Speed Limit")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: qsTr("Speed:")
                    font.pixelSize: 14
                    color: GTheme.textRegular
                    Layout.preferredWidth: 60
                }

                GTextField {
                    id: lowestSpeedInput
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    text: SettingsManager.qLowestSpeedLimit.toString()
                    placeholderText: "0"

                    // 只允许输入数字
                    validator: IntValidator {
                        bottom: 0
                        top: 1024  // 1 MB/s
                    }
                }

                Text {
                    text: qsTr("KB/s (0 = disabled, disconnect if speed is lower for 60s)")
                    font.pixelSize: 12
                    color: GTheme.textPlaceholder
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
                    maxOverallDownloadInput.text = "0"
                    maxOverallUploadInput.text = "0"
                    lowestSpeedInput.text = "0"

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
                    // 解析输入值
                    let downloadLimit = parseInt(maxOverallDownloadInput.text) || 0
                    let uploadLimit = parseInt(maxOverallUploadInput.text) || 0
                    let lowestSpeed = parseInt(lowestSpeedInput.text) || 0

                    // 验证范围
                    if (downloadLimit < 0 || downloadLimit > 102400) {
                        statusText.text = qsTr("✗ Download limit must be between 0 and 102400 KB/s")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("Invalid download limit!"), 3000)
                        return
                    }

                    if (uploadLimit < 0 || uploadLimit > 10240) {
                        statusText.text = qsTr("✗ Upload limit must be between 0 and 10240 KB/s")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("Invalid upload limit!"), 3000)
                        return
                    }

                    if (lowestSpeed < 0 || lowestSpeed > 1024) {
                        statusText.text = qsTr("✗ Lowest speed must be between 0 and 1024 KB/s")
                        statusText.color = GTheme.dangerColor
                        ToastManager.ShowError(qsTr("Invalid lowest speed!"), 3000)
                        return
                    }

                    // 检查是否有更改
                    let originalDownload = SettingsManager.qMaxOverallDownloadLimit
                    let originalUpload = SettingsManager.qMaxOverallUploadLimit
                    let originalLowest = SettingsManager.qLowestSpeedLimit

                    let hasChanges = (downloadLimit !== originalDownload) ||
                                   (uploadLimit !== originalUpload) ||
                                   (lowestSpeed !== originalLowest)

                    if (hasChanges) {
                        // 保存设置（会自动通过 RPC 更新 aria2c）
                        SettingsManager.SetAria2MaxOverallDownloadLimit(downloadLimit)
                        SettingsManager.SetAria2MaxOverallUploadLimit(uploadLimit)
                        SettingsManager.SetAria2LowestSpeedLimit(lowestSpeed)

                        ToastManager.ShowSuccess(
                            qsTr("✓ Speed control settings saved and applied successfully!"),
                            3000
                        )

                        statusText.text = qsTr("✓ Settings saved: Download=%1, Upload=%2, LowestSpeed=%3 KB/s")
                            .arg(downloadLimit).arg(uploadLimit).arg(lowestSpeed)
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
    }
}
