import QtQuick
import QtQuick.Controls
import gdl.sdk

SpinBox {
    id: control
    value: 50
    editable: true
    LayoutMirroring.enabled: false
    // 规格化：尺寸 large/default/small
    property string size: "default"
    readonly property int implicitH: (size === "large" ? 40 : (size === "small" ? 24 : 32))
    readonly property int radiusPx: 4

    // 内部属性用于颜色管理（基于 GTheme/ElementPlusColors 收敛）
    readonly property var colors: {
        const darkTheme = {
            background: GTheme.fillBase,
            border: GTheme.borderBase,
            borderFocus: GTheme.primaryColor,
            button: GTheme.fillLight,
            buttonHover: GTheme.fillBase,
            buttonPressed: GTheme.borderLight,
            arrow: GTheme.textSecondary,
            arrowHover: GTheme.textPrimary,
            text: GTheme.textPrimary
        }

        const lightTheme = {
            background: GTheme.bgWhite,
            border: GTheme.borderBase,
            borderFocus: GTheme.primaryColor,
            button: GTheme.fillLight,
            buttonHover: GTheme.fillBase,
            buttonPressed: GTheme.borderLight,
            arrow: GTheme.textRegular,
            arrowHover: GTheme.primaryColor,
            text: GTheme.textRegular
        }

        return GTheme.dark ? darkTheme : lightTheme
    }

    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: control.enabled ? colors.text : GTheme.textDisabled
        selectionColor: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(5)
        selectedTextColor: colors.text
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: Rectangle {
        x: parent.width - width - 2
        y: 2
        height: parent.height/2 - 2
        width: height
        color: control.enabled ? (control.up.pressed ? colors.buttonPressed : control.up.hovered ? colors.buttonHover : colors.button) : GTheme.fillLighter
        border.color: control.enabled ? colors.border : GTheme.borderBase

        // 添加颜色过渡动画
        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Canvas {
            id: upArrow
            anchors.fill: parent
            contextType: "2d"
            
            // 绘制向上箭头
            onPaint: {
                context.reset()
                context.beginPath()
                context.moveTo(width * 0.3, height * 0.6)
                context.lineTo(width * 0.5, height * 0.4)
                context.lineTo(width * 0.7, height * 0.6)
                context.lineWidth = 1.5
                context.strokeStyle = control.up.hovered ? colors.arrowHover : colors.arrow
                context.stroke()
            }
        }

        // 监听hover状态变化重绘箭头
        HoverHandler {
            onHoveredChanged: upArrow.requestPaint()
        }
    }

    down.indicator: Rectangle {
        x: parent.width - width - 2
        y: parent.height - height - 2
        height: parent.height/2 - 2
        width: height
        color: control.enabled ? (control.down.pressed ? colors.buttonPressed : control.down.hovered ? colors.buttonHover : colors.button) : GTheme.fillLighter
        border.color: control.enabled ? colors.border : GTheme.borderBase

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Canvas {
            id: downArrow
            anchors.fill: parent
            contextType: "2d"
            
            // 绘制向下箭头
            onPaint: {
                context.reset()
                context.beginPath()
                context.moveTo(width * 0.3, height * 0.4)
                context.lineTo(width * 0.5, height * 0.6)
                context.lineTo(width * 0.7, height * 0.4)
                context.lineWidth = 1.5
                context.strokeStyle = control.down.hovered ? colors.arrowHover : colors.arrow
                context.stroke()
            }
        }

        HoverHandler {
            onHoveredChanged: downArrow.requestPaint()
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: control.implicitH
        color: control.enabled ? colors.background : GTheme.fillLighter
        border.color: control.focus ? colors.borderFocus : colors.border
        radius: control.radiusPx
        
        // 添加边框颜色过渡动画
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
}
