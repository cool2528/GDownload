import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格百度网盘设置卡片
GCard {
    id: baiduCookieManager
    Layout.fillWidth: true
    Layout.preferredHeight: 120
    outlined: true
    padding: 16  // Element Plus 标准内边距

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // 卡片标题和描述
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("Baidu Netdisk Integration")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure Baidu Netdisk cookie for link parsing")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // Cookie 输入区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            // 标签区域
            ColumnLayout {
                Layout.preferredWidth: 100
                spacing: 4

                Text {
                    text: qsTr("Cookie")
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }

                Text {
                    text: qsTr("BDUSS value")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                }
            }

            GTextField {
                id: baiduCookie
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: SettingsManager.qBaiduPanCookies
                placeholderText: qsTr("Paste your Baidu Netdisk cookie here")
            }

            GButton {
                id: baiduCookieButton
                type: 1
                text: qsTr("Save")
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                onClicked: {
                    if(baiduCookie.text != ""){
                        let baidu_cookies = Utils.removeNewline(baiduCookie.text)
                        SettingsManager.SetBaiduPanCookies(baidu_cookies)
                        ToastManager.ShowSuccess(qsTr("Baidu cookie saved successfully"))
                    }
                }
            }
        }
    }
}
