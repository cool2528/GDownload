import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

// 通用卡片容器：统一背景/圆角/边框/hover/selected/disabled
Control {
    id: card

    // 交互属性（Control 自带 hoverEnabled/hovered）
    hoverEnabled: true
    property bool selected: false
    property bool outlined: false
    property bool disabled: false

    // 柔和投影(V5 消费级卡片高级感):opt-in,默认关闭。
    // 仅在非列表的少量卡片上开启;列表逐行卡片为性能不开启。
    // 开启时关闭 clip,否则阴影会被自身边界裁掉。
    property bool shadow: false

    // 消费级卡片变体:基于 GTheme 的低饱和表面,不引入页面私有颜色
    property string variant: "default"   // default | muted | elevated | accentPrimary | accentSuccess | accentWarning | accentInfo
    property bool interactive: true
    property bool compact: false

    // 外观属性
    // 消费级卡片默认大圆角(V5 设计稿),工具型场景调用方可覆盖为 radiusBase
    property int radius: GTheme.radiusLarge
    padding: 2
    clip: !shadow

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

        // 柔和投影:走 QtQuick.Effects 原生 MultiEffect(暗色更深、浅色更轻)
        layer.enabled: card.shadow
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: GTheme.dark ? Qt.rgba(0, 0, 0, 0.55) : Qt.rgba(0.11, 0.15, 0.22, 0.20)
            shadowBlur: 0.7
            shadowVerticalOffset: GTheme.spaceSM
            autoPaddingEnabled: true
        }
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
