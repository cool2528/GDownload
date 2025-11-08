import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

ComboBox {
    id: control
    // Element Plus 规格化 API
    // size: large | default | small
    property string size: "default"
    // status: normal | success | warning | danger（扩展状态支持）
    property string status: "normal"
    // clearable: 支持一键清空（Element Plus 特性）
    property bool clearable: false
    // placeholder: 占位文本
    property string placeholder: "Please select"

    readonly property int implicitH: (size === "large" ? 40 : (size === "small" ? 24 : 32))
    readonly property int radiusPx: 4
    readonly property int fontPx: (size === "large" ? 16 : (size === "small" ? 12 : 14))
    property int maxPopHeight: 200
    // Element Plus 风格颜色管理
    readonly property var colors: {
        const statusColors = {
            normal: {
                border: GTheme.borderBase,
                borderHover: GTheme.primaryColor,
                borderFocus: GTheme.primaryColor
            },
            success: {
                border: GTheme.borderBase,
                borderHover: GTheme.successColor,
                borderFocus: GTheme.successColor
            },
            warning: {
                border: GTheme.borderBase,
                borderHover: GTheme.warningColor,
                borderFocus: GTheme.warningColor
            },
            danger: {
                border: GTheme.borderBase,
                borderHover: GTheme.dangerColor,
                borderFocus: GTheme.dangerColor
            }
        }
        return statusColors[status] || statusColors.normal
    }

    delegate: ItemDelegate {
        width: control.width
        height: 32  // Element Plus 标准选项高度

        contentItem: Text {
            text: control.textRole
                ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole])
                : modelData
            color: {
                // 使用统一的状态判断，避免频繁切换
                const isHighlighted = control.highlightedIndex === index
                const isSelected = control.currentIndex === index

                if (isHighlighted || isSelected) {
                    return colors.borderFocus
                }
                return GTheme.dark ? GTheme.textPrimary : GTheme.textRegular
            }
            font.pixelSize: control.fontPx
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            leftPadding: 12
            rightPadding: 12
        }

        // 简化高亮逻辑，避免状态冲突
        highlighted: control.highlightedIndex === index
        hoverEnabled: true  // 启用悬停事件

        background: Rectangle {
            id: itemBackground
            // 简化颜色逻辑，优先级：高亮 > 悬停 > 选中 > 普通
            readonly property bool isHighlighted: control.highlightedIndex === index
            readonly property bool isSelected: control.currentIndex === index
            readonly property bool isHovered: parent.hovered
            readonly property bool isDark: GTheme.dark

            color: {
                if (isHighlighted) {
                    return isDark ? GTheme.fillLight : GTheme.primaryLight(1)
                }
                if (isHovered) {
                    // hover 高亮色
                    return isDark ? GTheme.fillBase : GTheme.fillLighter
                }
                if (isSelected) {
                    return isDark ? GTheme.fillBase : GTheme.fillLight
                }
                // 普通状态使用背景色，不透明
                return isDark ? GTheme.bgElevated : GTheme.bgWhite
            }

            // 使用更稳定的颜色过渡
            Behavior on color {
                enabled: !itemBackground.isHighlighted  // 使用完整路径引用
                ColorAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: control
            function onPressedChanged() { canvas.requestPaint(); }
        }

        Connections {
            target: GTheme
            function onDarkChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = GTheme.dark ?
                (control.pressed ? GTheme.textSecondary : GTheme.textPrimary) :
                (control.pressed ? GTheme.textSecondary : GTheme.textRegular)
            context.fill();
        }
    }

    contentItem: Text {
        leftPadding: 12  // Element Plus 标准内边距
        rightPadding: control.indicator.width + control.spacing + 8

        text: control.displayText || control.placeholder
        font.pixelSize: control.fontPx
        color: {
            if (!control.enabled) {
                return GTheme.textDisabled
            }
            if (!control.displayText) {
                // placeholder 样式
                return GTheme.textPlaceholder
            }
            return GTheme.dark ? GTheme.textPrimary : GTheme.textRegular
        }
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        // 文本颜色过渡
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: control.implicitH
        color: {
            if (!control.enabled) {
                return GTheme.fillLighter
            }
            return GTheme.dark ? GTheme.fillBase : GTheme.bgWhite
        }
        border.color: {
            if (!control.enabled) {
                return GTheme.borderLighter
            }
            if (control.visualFocus) {
                return colors.borderFocus
            }
            if (control.hovered || control.pressed) {
                return colors.borderHover
            }
            return colors.border
        }
        border.width: control.visualFocus ? 2 : 1
        radius: control.radiusPx

        // Element Plus focus 阴影效果
        layer.enabled: control.visualFocus
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: {
                const focusColor = colors.borderFocus
                return Qt.rgba(
                    Qt.colorEqual(focusColor, focusColor) ? focusColor.r : 0.25,
                    Qt.colorEqual(focusColor, focusColor) ? focusColor.g : 0.59,
                    Qt.colorEqual(focusColor, focusColor) ? focusColor.b : 1.0,
                    0.2
                )
            }
            shadowBlur: 6
            shadowVerticalOffset: 0
            shadowHorizontalOffset: 0
        }

        // 优化的颜色过渡动画，避免闪烁
        Behavior on color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
        Behavior on border.width {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: maxPopHeight < contentItem.implicitHeight ? maxPopHeight : contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            
            // 优化高亮索引更新，减少闪烁
            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < count) {
                    // 确保项目可见
                    positionViewAtIndex(currentIndex, ListView.Contain)
                }
            }
            
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: GTheme.dark ? GTheme.fillBase : GTheme.bgWhite
            border.color: GTheme.borderBase
            radius: control.radiusPx

            // Element Plus 下拉面板阴影
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Qt.rgba(0, 0, 0, 0.12)
                shadowBlur: 12
                shadowVerticalOffset: 4
                shadowHorizontalOffset: 0
            }

            // 面板展开动画
            scale: control.popup.visible ? 1.0 : 0.95
            opacity: control.popup.visible ? 1.0 : 0.0

            Behavior on scale {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
}
