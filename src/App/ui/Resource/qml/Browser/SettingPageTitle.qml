import QtQuick
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格页面标题(仅用于 3 个主页:Basic/Advanced/Lab)
// type:0=Basic / 1=Advanced / 2=Lab;其余子页用各自 SettingCard 标题,不用本组件
// 颜色/尺寸/间距/字号一律取自 GTheme 令牌,零魔法数字
Item {
    property int type: 0

    // 标题区固定高度:标题字号 + 描述字号 + 行间留白(令牌组合,保留原视觉高度)
    height: GTheme.fontTitle + GTheme.fontBody + GTheme.space2XL

    // Element Plus 风格标题文本
    Text {
        id: titleText
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: {
            if (type === 0) {
                return qsTr("Basic Settings")
            } else if (type === 1) {
                return qsTr("Advanced Settings")
            } else {
                return qsTr("Lab Settings")
            }
        }
        font.pixelSize: GTheme.fontTitle
        font.weight: GTheme.weightMedium
        color: GTheme.textPrimary
    }

    // 可选的页面描述
    Text {
        id: descriptionText
        anchors.left: parent.left
        anchors.top: titleText.bottom
        anchors.topMargin: GTheme.spaceXS
        text: {
            if (type === 0) {
                return qsTr("Configure basic download preferences")
            } else if (type === 1) {
                return qsTr("Advanced configuration options")
            } else {
                return qsTr("Experimental features and settings")
            }
        }
        font.pixelSize: GTheme.fontBody
        color: GTheme.textSecondary
        visible: text.length > 0
    }

    // Element Plus 风格分隔线(统一 Divider,色 borderLight)
    Divider {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: GTheme.spaceSM
        color: GTheme.borderLight
    }
}
