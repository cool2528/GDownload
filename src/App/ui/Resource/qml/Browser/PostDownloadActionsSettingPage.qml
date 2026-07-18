import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格下载完成后操作设置卡片(B 类,即时提交派)
// 控件事件直接 SetXxx 提交,无暂存-保存操作区,故不使用 SettingFormActions
// 骨架走 SettingCard;告警/示例框走 AlertTip(修复原 info 文字色幽灵令牌);分隔线统一 Divider
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字
SettingCard {
    id: postDownloadActionsSettingPage
    objectName: "postDownloadActionsSettingsCard"
    Layout.fillWidth: true
    Layout.minimumWidth: 0

    title: qsTr("Post-Download Actions")
    description: qsTr("Configure automated actions after download completes, fails, or starts")

    // 卡片正文:三段操作组 + 分隔线,段间距沿用原 16(spaceLG)
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceLG

        // ===== 下载完成后操作 =====
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("When Download Completes")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: completeActionComboBox
                objectName: "completeActionComboBox"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeDefault
                Accessible.name: qsTr("Action when download completes")
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
                // 仅用户交互提交,避免语言切换重建 model 时 currentIndex 复位被写回(Q2)
                onActivated: function(index) {
                    SettingsManager.SetOnCompleteAction(index)
                }
            }

            // 自定义完成命令(仅在选择"自定义命令"时显示)
            ColumnLayout {
                id: completeCommandSection
                objectName: "completeCommandSection"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM
                visible: completeActionComboBox.currentIndex === 4

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Custom Command")
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textPrimary
                }

                GTextField {
                    id: customCompleteCommandField
                    objectName: "customCompleteCommandField"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: GTheme.sizeDefault
                    Accessible.name: qsTr("Command to run when download completes")
                    placeholderText: qsTr("e.g., notify-send \"Download Complete\" \"{file}\"")
                    text: SettingsManager.qCustomCompleteCommand
                    onTextChanged: {
                        SettingsManager.SetCustomCompleteCommand(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Available variables:")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        font.weight: GTheme.weightMedium
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("  {file} - Downloaded file path") + "\n" +
                              qsTr("  {dir} - Download directory path") + "\n" +
                              qsTr("  {gid} - Download task ID")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        lineHeight: 1.5
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }

                    // 示例框:原裸 Text 的 info 文字色幽灵令牌 → AlertTip(info)
                    AlertTip {
                        Layout.fillWidth: true
                        severity: "info"
                        text: qsTr("Example:") + "\n" +
                              "  Windows: powershell -Command \"Write-Host '{file}'\"\n" +
                              "  macOS/Linux: /usr/local/bin/process.sh \"{file}\" \"{dir}\""
                    }
                }
            }
        }

        // 分隔线(统一 Divider,色 borderLight)
        Divider {
            Layout.fillWidth: true
        }

        // ===== 下载失败后操作 =====
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("When Download Fails")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: errorActionComboBox
                objectName: "errorActionComboBox"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeDefault
                Accessible.name: qsTr("Action when download fails")
                model: [
                    qsTr("Do Nothing"),
                    qsTr("Play Sound"),
                    qsTr("Run Custom Command")
                ]
                currentIndex: SettingsManager.qOnErrorAction
                onActivated: function(index) {
                    SettingsManager.SetOnErrorAction(index)
                }
            }

            // 自定义错误命令(仅在选择"自定义命令"时显示)
            ColumnLayout {
                id: errorCommandSection
                objectName: "errorCommandSection"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM
                visible: errorActionComboBox.currentIndex === 2

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Custom Command")
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textPrimary
                }

                GTextField {
                    id: customErrorCommandField
                    objectName: "customErrorCommandField"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: GTheme.sizeDefault
                    Accessible.name: qsTr("Command to run when download fails")
                    placeholderText: qsTr("e.g., logger \"Download failed: {gid}\"")
                    text: SettingsManager.qCustomErrorCommand
                    onTextChanged: {
                        SettingsManager.SetCustomErrorCommand(text)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Available variables:")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        font.weight: GTheme.weightMedium
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("  {file} - Downloaded file path") + "\n" +
                              qsTr("  {dir} - Download directory path") + "\n" +
                              qsTr("  {gid} - Download task ID")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        lineHeight: 1.5
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }

                    // 示例框:原裸 Text 的 info 文字色幽灵令牌 → AlertTip(info)
                    AlertTip {
                        Layout.fillWidth: true
                        severity: "info"
                        text: qsTr("Example:") + "\n" +
                              "  Windows: powershell -Command \"Write-EventLog -LogName Application -Source GDownload -EventId 1001 -Message 'Failed: {gid}'\"\n" +
                              "  macOS/Linux: echo \"Download error: {file}\" >> ~/download_errors.log"
                    }
                }
            }
        }

        // 分隔线(统一 Divider,色 borderLight)
        Divider {
            Layout.fillWidth: true
        }

        // ===== 下载开始后操作 =====
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("When Download Starts")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            GComBoBox {
                id: startActionComboBox
                objectName: "startActionComboBox"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeDefault
                Accessible.name: qsTr("Action when download starts")
                model: [
                    qsTr("Do Nothing"),
                    qsTr("Play Sound")
                ]
                currentIndex: SettingsManager.qOnStartAction
                onActivated: function(index) {
                    SettingsManager.SetOnStartAction(index)
                }
            }
        }
    }
}
