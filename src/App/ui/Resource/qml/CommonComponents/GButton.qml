import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk

Button {
    id: control
    
    // 属性定义
    property int type: 0 // 0: Default, 1: Primary
    property real radius: 2
    property real fontSize: 14
    
    // 内部属性用于颜色管理
    readonly property var colors: {
        const darkTheme = {
            default: {
                bg: "#515151",
                bgHover: "#2d2d2d",
                bgChecked: "#404040",
                text: "#ffffff",
                textHover: "#5151f9",
                textChecked: "#5151f9",
                border: "#515151",
                borderHover: "#555555",
                borderChecked: "#5151f9"
            },
            primary: {
                bg: "#5151f9",
                bgHover: "#7171fa",
                bgChecked: "#4141e9",
                text: "#ffffff",
                textHover: "#ffffff",
                textChecked: "#ffffff",
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        }
        
        const lightTheme = {
            default: {
                bg: "#ffffff",
                bgHover: "#ededff",
                bgChecked: "#f5f5ff",
                text: "#55575b",
                textHover: "#5151f9",
                textChecked: "#5151f9",
                border: "#d9dbe3",
                borderHover: "#c7c7fe",
                borderChecked: "#5151f9"
            },
            primary: {
                bg: "#5151f9",
                bgHover: "#7171fa",
                bgChecked: "#4141e9",
                text: "#ffffff",
                textHover: "#ffffff",
                textChecked: "#ffffff",
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        }
        
        return GTheme.dark ? darkTheme : lightTheme
    }
    
    // 获取当前主题色
    readonly property var currentTheme: colors[type === 0 ? "default" : "primary"]
    
    background: Rectangle {
        radius: control.radius
        color: {
            if (!control.enabled) {
                return GTheme.dark ? "#404040" : "#f5f5f5"
            }
            if (control.checked) {
                return currentTheme.bgChecked
            }
            return control.hovered ? currentTheme.bgHover : currentTheme.bg
        }
        border.color: {
            if (!control.enabled) {
                return GTheme.dark ? "#353535" : "#e0e0e0"
            }
            if (control.checked) {
                return currentTheme.borderChecked
            }
            return control.hovered ? currentTheme.borderHover : currentTheme.border
        }
        border.width: control.checked ? 1.5 : 1
        
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on border.width { NumberAnimation { duration: 150 } }
    }

    contentItem: Text {
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font.pixelSize: control.fontSize
        color: {
            if (!control.enabled) {
                return GTheme.dark ? "#808080" : "#999999"
            }
            if (control.checked) {
                return currentTheme.textChecked
            }
            return control.hovered ? currentTheme.textHover : currentTheme.text
        }
        
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
    
    // 添加点击效果
    scale: control.pressed ? 0.98 : 1.0
    
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
        }
    }
}
