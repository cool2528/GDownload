import QtQuick
import QtQuick.Controls
import gdl.sdk

CheckBox {
    id: control
    checked: false
    
    // 内部属性用于颜色管理
    readonly property var colors: {
        const darkTheme = {
            background: "#525354",
            backgroundChecked: "#5f5ff9",
            border: "transparent",
            borderHover: "#5f5ff9",
            checkmark: "#ffffff",
            text: "#ffffff"
        }
        
        const lightTheme = {
            background: "#ffffff",
            backgroundChecked: "#5f5ff9", 
            border: "#dadde4",
            borderHover: "#5f5ff9",
            checkmark: "#ffffff",
            text: "#55575b"
        }
        
        return GTheme.dark ? darkTheme : lightTheme
    }

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 3
        color: control.checked ? colors.backgroundChecked : colors.background
        border.color: control.hovered ? colors.borderHover : colors.border
        border.width: control.hovered ? 1.5 : 1
        
        // 添加颜色过渡动画
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
        Behavior on border.width {
            NumberAnimation { duration: 150 }
        }

        Rectangle {
            width: parent.width - 4
            height: parent.height - 4
            anchors.centerIn: parent
            color: "transparent"
            visible: control.checked
            radius: 2
            
            Canvas {
                id: checkmark
                anchors.fill: parent
                visible: control.checked
                
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    
                    // 设置绘制样式
                    ctx.strokeStyle = colors.checkmark
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    
                    // 计算绘制区域
                    var w = width
                    var h = height
                    var padding = w * 0.2
                    
                    // 绘制对勾
                    ctx.beginPath()
                    ctx.moveTo(padding, h * 0.5)
                    ctx.lineTo(w * 0.4, h * 0.7)
                    ctx.lineTo(w - padding, h * 0.3)
                    ctx.stroke()
                }
                
                // 添加勾选动画
                scale: control.checked ? 1 : 0
                Behavior on scale {
                    NumberAnimation { 
                        duration: 200
                        easing.type: Easing.OutBack
                    }
                }
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: colors.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
        
        // 添加文字颜色过渡
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    // 添加鼠标悬停效果
    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
    
    // 添加点击效果
    scale: control.down ? 0.95 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }
}
