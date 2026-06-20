import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格百度网盘 Cookie 设置卡片(B 类,即时提交派)
// 控件事件直接 SetBaiduPanCookies 提交,无暂存-保存操作区,故不使用 SettingFormActions
// 骨架走 SettingCard(替代原 GCard + 内层 ColumnLayout 双重 16 边距,改为单层 spaceLG 内边距)
// Cookie 输入行为复合布局:双行标签列 + 填充输入框 + 行内保存按钮,
// 不符合 SettingRow 的单行 label+control+hint 三段式,故保留令牌化的自定义 RowLayout
// 颜色/尺寸/间距/字号/圆角全走 GTheme 令牌,零魔法数字
SettingCard {
    id: baiduCookieManager
    Layout.fillWidth: true

    title: qsTr("Baidu Netdisk Integration")
    description: qsTr("Configure Baidu Netdisk cookie for link parsing")

    // 页面级布局常量(非设计令牌):标签列宽 / 保存按钮宽
    readonly property int labelColumnWidth: 100
    readonly property int saveButtonWidth: 80

    // Cookie 输入区:标签列(主标签 + 副说明)+ 输入框 + 保存按钮
    RowLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceLG

        // 标签列:主标签 Cookie + 副说明 BDUSS value
        ColumnLayout {
            Layout.preferredWidth: baiduCookieManager.labelColumnWidth
            spacing: GTheme.spaceXS

            Text {
                text: qsTr("Cookie")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("BDUSS value")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }
        }

        GTextField {
            id: baiduCookie
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault
            text: SettingsManager.qBaiduPanCookies
            placeholderText: qsTr("Paste your Baidu Netdisk cookie here")
        }

        GButton {
            id: baiduCookieButton
            type: 1
            text: qsTr("Save")
            Layout.preferredWidth: baiduCookieManager.saveButtonWidth
            Layout.preferredHeight: GTheme.sizeDefault
            onClicked: {
                if (baiduCookie.text != "") {
                    let baidu_cookies = Utils.removeNewline(baiduCookie.text)
                    SettingsManager.SetBaiduPanCookies(baidu_cookies)
                    ToastManager.ShowSuccess(qsTr("Baidu cookie saved successfully"))
                }
            }
        }
    }
}
