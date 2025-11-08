import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 配置助手卡片 - 帮助用户配置浏览器插件连接
GCard {
    id: configCard
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16

    // 从 SettingsManager 读取 aria2c 配置
    readonly property string rpcUrl: "ws://127.0.0.1:" + SettingsManager.qRpcListenPort + "/jsonrpc"
    readonly property string rpcSecret: SettingsManager.qRpcSecret

    // 复制到剪贴板的辅助函数
    function copyToClipboard(text, successMessage) {
        UtilsToolsManager.SetClipboardText(text)
        ToastManager.ShowSuccess(successMessage, 2000)
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 20

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                spacing: 8

                Text {
                    text: "\uE713"  // settings icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 18
                    color: GTheme.primaryColor
                }

                Text {
                    text: qsTr("Configuration Helper")
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }
            }

            Text {
                text: qsTr("Copy these settings to your browser extension")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 当前 GDownload 设置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            Text {
                text: qsTr("Current GDownload Settings:")
                font.pixelSize: 13
                font.weight: Font.Medium
                color: GTheme.textRegular
            }

            // WebSocket URL 配置
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: qsTr("WebSocket URL:")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    GTextField {
                        id: urlField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: rpcUrl
                        readOnly: true
                        selectByMouse: true
                    }

                    GButton {
                        text: qsTr("Copy")
                        type: 2
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 36
                        onClicked: {
                            copyToClipboard(rpcUrl, qsTr("✓ WebSocket URL copied!"))
                        }
                    }
                }
            }

            // RPC Secret 配置
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: qsTr("RPC Secret:")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    GTextField {
                        id: secretField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: rpcSecret
                        readOnly: true
                        selectByMouse: true
                    }

                    GButton {
                        text: qsTr("Copy")
                        type: 2
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 36
                        onClicked: {
                            copyToClipboard(rpcSecret, qsTr("✓ RPC Secret copied!"))
                        }
                    }
                }
            }

            // 状态指示器
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: statusLayout.implicitHeight + 16
                color: Qt.rgba(GTheme.successColor.r, GTheme.successColor.g, GTheme.successColor.b, 0.1)
                radius: 4

                RowLayout {
                    id: statusLayout
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // 状态圆点
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: GTheme.successColor

                        // 脉冲动画
                        SequentialAnimation on opacity {
                            running: true
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: 1000 }
                            NumberAnimation { from: 0.3; to: 1.0; duration: 1000 }
                        }
                    }

                    Text {
                        text: qsTr("Status: Connected (aria2c running)")
                        font.pixelSize: 12
                        color: GTheme.successColor
                        font.weight: Font.Medium
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

        // 使用说明
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            RowLayout {
                spacing: 8

                Text {
                    text: "\uEA80"  // lightbulb icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 16
                    color: GTheme.warningColor
                }

                Text {
                    text: qsTr("How to use:")
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }
            }

            Text {
                text: qsTr("1. Copy the settings above using the Copy buttons")
                font.pixelSize: 12
                color: GTheme.textRegular
                leftPadding: 24
            }

            Text {
                text: qsTr("2. Open your browser extension options page")
                font.pixelSize: 12
                color: GTheme.textRegular
                leftPadding: 24
            }

            Text {
                text: qsTr("3. Paste the values into the corresponding fields")
                font.pixelSize: 12
                color: GTheme.textRegular
                leftPadding: 24
            }

            Text {
                text: qsTr("4. Click 'Test Connection' to verify")
                font.pixelSize: 12
                color: GTheme.textRegular
                leftPadding: 24
            }
        }

        // 操作按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 8

            GButton {
                text: qsTr("📋 Copy All Settings")
                type: 1
                Layout.preferredWidth: 160
                Layout.preferredHeight: 36
                onClicked: {
                    var allSettings = "WebSocket URL: " + rpcUrl + "\n" +
                                    "RPC Secret: " + rpcSecret
                    copyToClipboard(allSettings, qsTr("✓ All settings copied!"))
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Need help? Check the FAQ below")
                font.pixelSize: 11
                color: GTheme.textPlaceholder
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // 重要提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: importantLayout.implicitHeight + 16
            color: Qt.rgba(GTheme.warningColor.r, GTheme.warningColor.g, GTheme.warningColor.b, 0.1)
            radius: 4
            border.width: 1
            border.color: Qt.rgba(GTheme.warningColor.r, GTheme.warningColor.g, GTheme.warningColor.b, 0.3)

            RowLayout {
                id: importantLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: "\uE7BA"  // alert icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 16
                    color: GTheme.warningColor
                    Layout.alignment: Qt.AlignTop
                }

                Text {
                    text: qsTr("Important: Keep GDownload running for the browser extension to work. The extension connects directly to aria2c via these settings.")
                    font.pixelSize: 11
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
            }
        }
    }
}
