import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 配置助手卡片 - 帮助用户配置浏览器插件连接
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字(装饰尺寸以局部常量标注)
GCard {
    id: configCard
    Layout.fillWidth: true
    // 高度 = 内容隐式高度 + 上下 GCard padding(各 spaceLG,单层边距,消除原双重 16 边距)
    Layout.preferredHeight: contentLayout.implicitHeight + 2 * GTheme.spaceLG
    outlined: true
    padding: GTheme.spaceLG

    // 局部布局/装饰常量(EP 令牌无直接对应,显式标注用途,值由令牌派生)
    readonly property int copyButtonWidth: GTheme.spaceLG * 5       // 单项复制按钮宽(=80)
    readonly property int copyAllButtonWidth: GTheme.spaceLG * 10   // 一键复制按钮宽(=160)
    readonly property int statusDotSize: 10                         // 状态圆点直径(装饰元素,EP 令牌无对应尺寸)
    readonly property int pulseDuration: GTheme.durationSlow * 4    // 脉冲动画半周期(呼吸效果,慢于通用动效令牌)

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
        anchors.margins: 0
        spacing: GTheme.spaceXL

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            RowLayout {
                spacing: GTheme.spaceSM

                Text {
                    text: "\uE713"  // settings icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: GTheme.fontTitle
                    color: GTheme.primaryColor
                }

                Text {
                    text: qsTr("Configuration Helper")
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }
            }

            Text {
                text: qsTr("Copy these settings to your browser extension")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }
        }

        // 当前 GDownload 设置
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceLG

            Text {
                text: qsTr("Current GDownload Settings:")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textRegular
            }

            // WebSocket URL 配置
            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                Text {
                    text: qsTr("WebSocket URL:")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                }

                RowLayout {
                    spacing: GTheme.spaceSM
                    Layout.fillWidth: true

                    GTextField {
                        id: urlField
                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.sizeDefault
                        text: rpcUrl
                        readOnly: true
                        selectByMouse: true
                    }

                    GButton {
                        text: qsTr("Copy")
                        type: 2
                        Layout.preferredWidth: configCard.copyButtonWidth
                        Layout.preferredHeight: GTheme.sizeDefault
                        onClicked: {
                            copyToClipboard(rpcUrl, qsTr("✓ WebSocket URL copied!"))
                        }
                    }
                }
            }

            // RPC Secret 配置
            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                Text {
                    text: qsTr("RPC Secret:")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                }

                RowLayout {
                    spacing: GTheme.spaceSM
                    Layout.fillWidth: true

                    GTextField {
                        id: secretField
                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.sizeDefault
                        text: rpcSecret
                        readOnly: true
                        selectByMouse: true
                    }

                    GButton {
                        text: qsTr("Copy")
                        type: 2
                        Layout.preferredWidth: configCard.copyButtonWidth
                        Layout.preferredHeight: GTheme.sizeDefault
                        onClicked: {
                            copyToClipboard(rpcSecret, qsTr("✓ RPC Secret copied!"))
                        }
                    }
                }
            }

            // 状态指示器(成功语义浅底,告警令牌 bgSuccess)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: statusLayout.implicitHeight + GTheme.spaceLG
                color: GTheme.bgSuccess
                radius: GTheme.radiusBase

                RowLayout {
                    id: statusLayout
                    anchors.fill: parent
                    anchors.margins: GTheme.spaceMD
                    spacing: GTheme.spaceSM

                    // 状态圆点(由 RowLayout 管理尺寸,用 Layout.preferred* 避免 UB)
                    Rectangle {
                        Layout.preferredWidth: configCard.statusDotSize
                        Layout.preferredHeight: configCard.statusDotSize
                        radius: configCard.statusDotSize / 2
                        color: GTheme.successColor

                        // 脉冲动画
                        SequentialAnimation on opacity {
                            running: true
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: configCard.pulseDuration }
                            NumberAnimation { from: 0.3; to: 1.0; duration: configCard.pulseDuration }
                        }
                    }

                    Text {
                        text: qsTr("Status: Connected (aria2c running)")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.successColor
                        font.weight: GTheme.weightMedium
                    }
                }
            }
        }

        // 分隔线(色 borderLight)
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        // 使用说明
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            RowLayout {
                spacing: GTheme.spaceSM

                Text {
                    text: "\uEA80"  // lightbulb icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: GTheme.fontSubtitle
                    color: GTheme.warningColor
                }

                Text {
                    text: qsTr("How to use:")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }
            }

            Text {
                text: qsTr("1. Copy the settings above using the Copy buttons")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                leftPadding: GTheme.space2XL
            }

            Text {
                text: qsTr("2. Open your browser extension options page")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                leftPadding: GTheme.space2XL
            }

            Text {
                text: qsTr("3. Paste the values into the corresponding fields")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                leftPadding: GTheme.space2XL
            }

            Text {
                text: qsTr("4. Click 'Test Connection' to verify")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                leftPadding: GTheme.space2XL
            }
        }

        // 操作按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD
            Layout.topMargin: GTheme.spaceSM

            GButton {
                text: qsTr("📋 Copy All Settings")
                type: 1
                Layout.preferredWidth: configCard.copyAllButtonWidth
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    var allSettings = "WebSocket URL: " + rpcUrl + "\n" +
                                    "RPC Secret: " + rpcSecret
                    copyToClipboard(allSettings, qsTr("✓ All settings copied!"))
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Need help? Check the FAQ below")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textPlaceholder
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // 重要提示(警示语义浅底+边框,告警令牌 bgWarning/borderWarning)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: importantLayout.implicitHeight + GTheme.spaceLG
            color: GTheme.bgWarning
            radius: GTheme.radiusBase
            border.width: 1
            border.color: GTheme.borderWarning

            RowLayout {
                id: importantLayout
                anchors.fill: parent
                anchors.margins: GTheme.spaceMD
                spacing: GTheme.spaceSM

                Text {
                    text: "\uE7BA"  // alert icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: GTheme.fontSubtitle
                    color: GTheme.warningColor
                    Layout.alignment: Qt.AlignTop
                }

                Text {
                    text: qsTr("Important: Keep GDownload running for the browser extension to work. The extension connects directly to aria2c via these settings.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
            }
        }
    }
}
