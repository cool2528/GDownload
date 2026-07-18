import QtQuick
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格页面标题(仅用于 3 个主页:Basic/Advanced/Lab)
// type:0=Basic / 1=Advanced / 2=Lab;其余子页用各自 SettingCard 标题,不用本组件
// 颜色/尺寸/间距/字号一律取自 GTheme 令牌,零魔法数字
Item {
    property int type: 0

    // 标题区高度:标题 + 描述 + 行间距 + 与分隔线的下方留白(令牌组合)
    height: GTheme.fontTitle + GTheme.fontBody + GTheme.spaceXS + GTheme.space3XL

    // Element Plus 风格标题文本(顶部对齐,保证描述与分隔线之间有足够间距)
    Text {
        id: titleText
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: GTheme.spaceXS
        text: {
            if (type === 0) {
                return qsTr("Basic Settings")
            } else if (type === 1) {
                return qsTr("Advanced Settings")
            } else if (type === 2) {
                return qsTr("Lab Settings")
            } else {
                return qsTr("Plugin Market")
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
            } else if (type === 2) {
                return qsTr("Experimental features and settings")
            } else {
                return qsTr("Browse, install and update netdisk parser plugins")
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
