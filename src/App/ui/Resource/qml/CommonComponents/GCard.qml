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

    // 消费级卡片变体:基于 GTheme 的低饱和表面,不引入页面私有颜色
    property string variant: "default"   // default | muted | elevated | accentPrimary | accentSuccess | accentWarning | accentInfo
    property bool interactive: true
    property bool compact: false

    // 外观属性
    property int radius: 4
    padding: 2
    clip: true

    readonly property int resolvedPadding: compact ? GTheme.spaceSM : padding

    function variantBackground() {
        switch (variant) {
        case "muted": return GTheme.fillLighter
        case "elevated": return GTheme.bgElevated
        case "accentPrimary": return GTheme.primaryLight(9)
        case "accentSuccess": return GTheme.bgSuccess
        case "accentWarning": return GTheme.bgWarning
        case "accentInfo": return GTheme.bgInfo
        default: return GTheme.bgBase
        }
    }

    function variantBorder() {
        if (selected) return GTheme.primaryColor
        switch (variant) {
        case "accentPrimary": return GTheme.primaryLight(7)
        case "accentSuccess": return GTheme.borderSuccess
        case "accentWarning": return GTheme.borderWarning
        case "accentInfo": return GTheme.borderInfo
        default: return outlined ? GTheme.borderLight : "transparent"
        }
    }

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                             contentItem ? contentItem.implicitWidth + padding : 0)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                              contentItem ? contentItem.implicitHeight + padding : 0)

    background: Rectangle {
        id: bg
        radius: card.radius
        // Ant Design + VS Code 配色方案
        color: {
            if (card.disabled) return GTheme.fillLighter
            if ((hoverHandler.hovered && card.hoverEnabled && card.interactive) || card.selected)
                return card.variant === "default" ? GTheme.bgElevated : card.variantBackground()
            return card.variantBackground()
        }
        border.width: card.selected ? 2 : (card.outlined || card.variant !== "default" ? 1 : 0)
        border.color: card.variantBorder()
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
