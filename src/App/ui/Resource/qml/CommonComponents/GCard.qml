import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

// Aurora 通用表面容器：统一 surface、边框、交互态、焦点和 elevation。
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

    // 表面变体全部映射到 Aurora 语义令牌，不引入页面私有颜色。
    property string variant: "default"   // default | muted | elevated | accentPrimary | accentSuccess | accentWarning | accentInfo
    property bool interactive: true
    property bool compact: false

    // 外观属性
    // 消费级卡片默认大圆角(V5 设计稿),工具型场景调用方可覆盖为 radiusBase
    property int radius: GTheme.radiusLarge
    padding: GTheme.spaceLG
    clip: !shadow

    readonly property int resolvedPadding: compact ? Math.min(padding, GTheme.spaceSM) : padding

    function variantBackground() {
        switch (variant) {
        case "muted": return GTheme.fillLighter
        case "elevated": return GTheme.surfaceElevated
        case "accentPrimary": return GTheme.primaryLight(9)
        case "accentSuccess": return GTheme.bgSuccess
        case "accentWarning": return GTheme.bgWarning
        case "accentInfo": return GTheme.bgInfo
        default: return GTheme.surfaceBase
        }
    }

    function variantBorder() {
        if (selected) return GTheme.primaryColor
        switch (variant) {
        case "accentPrimary": return GTheme.primaryLight(7)
        case "accentSuccess": return GTheme.borderSuccess
        case "accentWarning": return GTheme.borderWarning
        case "accentInfo": return GTheme.borderInfo
        default: return outlined ? GTheme.borderBase : "transparent"
        }
    }

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                             contentItem ? contentItem.implicitWidth + resolvedPadding * 2 : 0)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                              contentItem ? contentItem.implicitHeight + resolvedPadding * 2 : 0)

    background: Rectangle {
        id: bg
        radius: card.radius
        color: {
            if (card.disabled) return GTheme.fillLighter
            if ((hoverHandler.hovered && card.hoverEnabled && card.interactive) || card.selected)
                return card.variant === "default" ? GTheme.surfaceElevated : GTheme.fillLight
            return card.variantBackground()
        }
        border.width: card.selected || card.activeFocus ? 2
                                                     : (card.outlined || card.variant !== "default" ? 1 : 0)
        border.color: card.activeFocus ? GTheme.focusRing : card.variantBorder()
        opacity: card.disabled ? 0.72 : 1.0

        Behavior on color {
            ColorAnimation { duration: GTheme.durationFast }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationFast }
        }

        // 柔和投影:走 QtQuick.Effects 原生 MultiEffect(暗色更深、浅色更轻)
        layer.enabled: card.shadow
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: GTheme.elevation2.color
            shadowBlur: Math.min(1.0, GTheme.elevation2.blur / 32)
            shadowHorizontalOffset: GTheme.elevation2.offsetX
            shadowVerticalOffset: GTheme.elevation2.offsetY
            autoPaddingEnabled: true
        }
    }

    contentItem: Item {
        anchors.fill: parent
        anchors.margins: card.resolvedPadding
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }

    HoverHandler {
        id: hoverHandler
        enabled: card.hoverEnabled
    }
}
