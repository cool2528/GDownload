import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格下载完成后操作设置卡片
GCard {
    id: postDownloadActionsSettingPage
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
                text: qsTr("Post-Download Actions")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure automated actions after download completes, fails, or starts")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 下载完成后操作
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("When Download Completes")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: completeActionComboBox
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: [
                    qsTr("Do Nothing"),
                    qsTr("Open File"),
                    qsTr("Open Directory"),
                    qsTr("Play Sound"),
                    qsTr("Run Custom Command"),
                    qsTr("Shutdown Computer"),
                    qsTr("Sleep Computer"),
                    qsTr("Restart Computer")
                ]
                currentIndex: SettingsManager.qOnCompleteAction
                onCurrentIndexChanged: {
                    SettingsManager.SetOnCompleteAction(currentIndex)
                }
            }

            // 自定义完成命令（仅在选择"自定义命令"时显示）
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: completeActionComboBox.currentIndex === 4

                Text {
                    text: qsTr("Custom Command")
                    font.pixelSize: 13
                    color: GTheme.textPrimary
                }

                GTextField {
                    id: customCompleteCommandField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    placeholderText: qsTr("e.g., notify-send \"Download Complete\" \"{file}\"")
                    text: SettingsManager.qCustomCompleteCommand
                    onTextChanged: {
                        SettingsManager.SetCustomCompleteCommand(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("Available variables:")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        font.weight: Font.Medium
                    }

                    Text {
                        text: qsTr("  {file} - Downloaded file path") + "\n" +
                              qsTr("  {dir} - Download directory path") + "\n" +
                              qsTr("  {gid} - Download task ID")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        lineHeight: 1.5
                    }

                    Text {
                        text: qsTr("Example:") + "\n" +
                              "  Windows: powershell -Command \"Write-Host '{file}'\"\n" +
                              "  macOS/Linux: /usr/local/bin/process.sh \"{file}\" \"{dir}\""
                        font.pixelSize: 12
                        color: GTheme.textInfo
                        lineHeight: 1.5
                        font.family: "Consolas, Monaco, monospace"
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

        // 下载失败后操作
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("When Download Fails")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: errorActionComboBox
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: [
                    qsTr("Do Nothing"),
                    qsTr("Play Sound"),
                    qsTr("Run Custom Command")
                ]
                currentIndex: SettingsManager.qOnErrorAction
                onCurrentIndexChanged: {
                    SettingsManager.SetOnErrorAction(currentIndex)
                }
            }

            // 自定义错误命令（仅在选择"自定义命令"时显示）
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8
                visible: errorActionComboBox.currentIndex === 2

                Text {
                    text: qsTr("Custom Command")
                    font.pixelSize: 13
                    color: GTheme.textPrimary
                }

                GTextField {
                    id: customErrorCommandField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    placeholderText: qsTr("e.g., logger \"Download failed: {gid}\"")
                    text: SettingsManager.qCustomErrorCommand
                    onTextChanged: {
                        SettingsManager.SetCustomErrorCommand(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("Available variables:")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        font.weight: Font.Medium
                    }

                    Text {
                        text: qsTr("  {file} - Downloaded file path") + "\n" +
                              qsTr("  {dir} - Download directory path") + "\n" +
                              qsTr("  {gid} - Download task ID")
                        font.pixelSize: 12
                        color: GTheme.textSecondary
                        lineHeight: 1.5
                    }

                    Text {
                        text: qsTr("Example:") + "\n" +
                              "  Windows: powershell -Command \"Write-EventLog -LogName Application -Source GDownload -EventId 1001 -Message 'Failed: {gid}'\"\n" +
                              "  macOS/Linux: echo \"Download error: {file}\" >> ~/download_errors.log"
                        font.pixelSize: 12
                        color: GTheme.textInfo
                        lineHeight: 1.5
                        font.family: "Consolas, Monaco, monospace"
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

        // 下载开始后操作
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("When Download Starts")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: startActionComboBox
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: [
                    qsTr("Do Nothing"),
                    qsTr("Play Sound")
                ]
                currentIndex: SettingsManager.qOnStartAction
                onCurrentIndexChanged: {
                    SettingsManager.SetOnStartAction(currentIndex)
                }
            }
        }
    }
}
