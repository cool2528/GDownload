import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk

SpinBox {
    id: control
    value: 50
    editable: true
    LayoutMirroring.enabled: false

    // 内部属性用于颜色管理
    readonly property var colors: {
        const darkTheme = {
            background: "#303030",
            border: "#404040",
            borderFocus: "#5151f9", 
            button: "#404040",
            buttonHover: "#505050",
            buttonPressed: "#606060",
            arrow: "#909090",
            arrowHover: "#ffffff",
            text: "#ffffff"
        }
        
        const lightTheme = {
            background: "#ffffff",
            border: "#d7dae2", 
            borderFocus: "#5151f9",
            button: "#f6f6f6",
            buttonHover: "#f0f0f0", 
            buttonPressed: "#e4e4e4",
            arrow: "#94969a",
            arrowHover: "#5151f9",
            text: "#7b7d80"
        }
        
        return GTheme.dark ? darkTheme : lightTheme
    }

    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: colors.text
        selectionColor: GTheme.dark ? "#505050" : "#acd2fe"
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
        color: control.up.pressed ? colors.buttonPressed : 
               control.up.hovered ? colors.buttonHover : colors.button
        border.color: colors.border

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
        color: control.down.pressed ? colors.buttonPressed :
               control.down.hovered ? colors.buttonHover : colors.button
        border.color: colors.border

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
        implicitHeight: 30
        color: colors.background
        border.color: control.focus ? colors.borderFocus : colors.border
        
        // 添加边框颜色过渡动画
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
    }
}
