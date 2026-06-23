import QtQuick
import QtQuick.Layouts
import gdl.sdk 1.0

/**
 * SettingCard - 设置页通用卡片容器
 * GCard 派生,替代设置页 B 类卡片骨架(原 GCard{outlined; padding:16} + 内层 ColumnLayout{margins:16}
 * 双重 16 边距)。约定见 spec Section 2.1:outlined + padding=spaceLG 单层边距;标题 fontSubtitle/
 * weightMedium/textPrimary,描述 fontCaption/textSecondary,标题描述组 spacing=spaceXS;内容槽接收
 * 调用方注入的 content。颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字。
 *
 * 用法:
 *   SettingCard {
 *       title: qsTr("Speed Control")
 *       description: qsTr("Set download/upload speed limits")
 *       ColumnLayout { ... }   // 内容槽(默认属性 content,可直接写子项)
 *   }
 */
GCard {
    id: root

    // ========== 公开属性 ==========

    // 卡片标题(调用方用 qsTr 包裹英文)
    property string title: ""

    // 卡片描述(标题下方副文本,调用方用 qsTr 包裹英文)
    property string description: ""

    // 内容槽:调用方注入的卡片正文(默认属性,可直接写子项或用 content: 显式赋值)
    default property alias content: contentSlot.children

    // ========== 派生 ==========

    // 是否存在标题/描述:控制标题组可见性及标题组与内容槽的间距
    readonly property bool hasHeader: root.title !== "" || root.description !== ""

    // ========== 卡片基底约定(spec Section 2.1)==========

    outlined: true
    padding: GTheme.spaceLG
    radius: GTheme.radiusRound
    // 设置卡片为静态容器,关闭 GCard 默认悬停高亮
    hoverEnabled: false
    // 浮起表面 + 柔和阴影:在设置面板上形成层级(暗色 #1D1D1D 高于面板 #141414),
    // 与下载页任务卡一致,贴合设计稿"卡片浮起"观感
    variant: "elevated"
    shadow: true

    // 修正隐式尺寸:GCard 基类隐式宽高按 +padding(单侧)计,会少算一侧导致内容被裁;
    // 此处按 +2*padding(双侧)计,使内容槽在隐式高度下完整显示
    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                           contentItem ? contentItem.implicitWidth + 2 * root.padding : 0)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                            contentItem ? contentItem.implicitHeight + 2 * root.padding : 0)

    // ========== 内容布局:标题组 + 内容槽 ==========

    contentItem: ColumnLayout {
        id: cardBody
        // 沿用 GCard 单层内边距模式:contentItem 以 anchors.margins=padding 内缩 spaceLG,
        // 替代原 GCard padding:16 + 内层 margins:16 的双重边距
        anchors.fill: parent
        anchors.margins: root.padding
        // 标题组与内容槽间距:有标题时 spaceLG,无标题时收为 0(避免内容顶部多余空隙)
        spacing: root.hasHeader ? GTheme.spaceLG : 0

        // 标题 + 描述组
        ColumnLayout {
            id: headerGroup
            Layout.fillWidth: true
            spacing: GTheme.spaceXS
            visible: root.hasHeader

            Text {
                Layout.fillWidth: true
                visible: root.title !== ""
                text: root.title
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: root.description !== ""
                text: root.description
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }

        // 内容槽(调用方 content 注入于此,纵向堆叠)
        ColumnLayout {
            id: contentSlot
            Layout.fillWidth: true
            spacing: GTheme.spaceMD
        }
    }
}
