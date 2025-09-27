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
    padding: 10
    clip: true

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                             contentItem ? contentItem.implicitWidth + padding * 2 : 0)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                              contentItem ? contentItem.implicitHeight + padding * 2 : 0)

    background: Rectangle {
        id: bg
        radius: card.radius
        color: card.disabled ? GTheme.fillLighter
                              : (hoverHandler.hovered && card.hoverEnabled ? GTheme.fillLight
                                                                          : (GTheme.dark ? "#2D2D2D" : GTheme.bgWhite))
        border.width: card.selected ? 2 : (card.outlined ? 1 : 0)
        border.color: card.selected ? GTheme.primaryColor : (card.outlined ? (GTheme.dark ? GTheme.borderBase : GTheme.borderLight) : "transparent")
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
