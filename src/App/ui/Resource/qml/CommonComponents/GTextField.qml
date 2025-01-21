import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk

TextField {
    id: control
    
    // 内部属性用于颜色管理
    readonly property var colors: {
        const darkTheme = {
            background: "#303030",
            border: "#484848",
            borderFocus: "#5151f9",
            text: "#ffffff",
            placeholder: "#9a9a9a",
            selection: "#505050",
            selectedText: "#ffffff"
        }
        
        const lightTheme = {
            background: "#ffffff", 
            border: "#d7dae2",
            borderFocus: "#5151f9",
            text: "#303133",
            placeholder: "#bababa",
            selection: "#e8f0fe",
            selectedText: "#303133"
        }
        
        return GTheme.dark ? darkTheme : lightTheme
    }

    // 基础属性设置
    font.pixelSize: 14
    selectByMouse: true
    selectedTextColor: colors.selectedText
    selectionColor: colors.selection
    color: colors.text
    placeholderTextColor: colors.placeholder

    // 背景设置
    background: Rectangle {
        implicitHeight: 30
        implicitWidth: 200
        color: colors.background
        border.color: control.activeFocus ? colors.borderFocus : colors.border
        border.width: control.activeFocus ? 2 : 1
        radius: 2

        // 添加边框颜色过渡动画
        Behavior on border.color {
            ColorAnimation { duration: 150 }
        }
        
        Behavior on border.width {
            NumberAnimation { duration: 150 }
        }
    }

    // 添加鼠标悬停效果
    HoverHandler {
        cursorShape: Qt.IBeamCursor
    }
    
    // 添加获得焦点时的缩放动画
    scale: activeFocus ? 1.02 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }
}
