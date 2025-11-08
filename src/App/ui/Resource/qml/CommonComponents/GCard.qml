import QtQuick
import QtQuick.Controls
import gdl.sdk

// 通用卡片容器：统一背景/圆角/边框/hover/selected/disabled
Control {
    id: card

    // 交互属性（Control 自带 hoverEnabled/hovered）
    hoverEnabled: true
    property bool selected: false
    property bool outlined: false
    property bool disabled: false

    // 外观属性
    property int radius: 4
    padding: 2
    clip: true

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                             contentItem ? contentItem.implicitWidth + padding : 0)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                              contentItem ? contentItem.implicitHeight + padding : 0)

    background: Rectangle {
        id: bg
        radius: card.radius
        // Ant Design + VS Code 配色方案
        color: {
            if (card.disabled) {
                return GTheme.fillLighter
            }

            if (hoverHandler.hovered && card.hoverEnabled) {
                // Ant Design (浅色): #FFFFFF 纯白高亮
                // VS Code (暗色): #2D2D30 活动项背景
                return GTheme.bgElevated
            }

            // Ant Design (浅色): #FAFAFA 浅灰卡片
            // VS Code (暗色): #252526 侧边栏色
            return GTheme.bgBase
        }
        border.width: card.selected ? 2 : (card.outlined ? 1 : 0)
        border.color: card.selected ? GTheme.primaryColor : (card.outlined ? GTheme.borderBase : "transparent")
    }

    contentItem: Item {
        anchors.fill: parent
        anchors.margins: card.padding
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }

    HoverHandler {
        id: hoverHandler
        enabled: card.hoverEnabled
    }
}
