import QtQuick
import QtQuick.Controls
import gdl.sdk

TextField {
    id: control
    // 规格化：尺寸 large/default/small
    property string size: "default"
    readonly property int implicitH: (size === "large" ? 40 : (size === "small" ? 24 : 32))
    readonly property int radiusPx: 4
    readonly property int fontPx: (size === "large" ? 16 : (size === "small" ? 12 : 14))
    
    // 内部属性用于颜色管理（基于 GTheme/ElementPlusColors 收敛）
    readonly property var colors: {
        const darkTheme = {
            background: GTheme.fillBase,
            border: GTheme.borderBase,
            borderFocus: GTheme.primaryColor,
            text: GTheme.textPrimary,
            placeholder: GTheme.textPlaceholder,
            selection: GTheme.fillLight,
            selectedText: GTheme.textPrimary
        }

        const lightTheme = {
            background: GTheme.bgWhite,
            border: GTheme.borderBase,
            borderFocus: GTheme.primaryColor,
            text: GTheme.textPrimary,
            placeholder: GTheme.textPlaceholder,
            selection: GTheme.primaryLight(5),
            selectedText: GTheme.textPrimary
        }

        return GTheme.dark ? darkTheme : lightTheme
    }

    // 基础属性设置
    font.pixelSize: fontPx
    selectByMouse: true
    selectedTextColor: colors.selectedText
    selectionColor: colors.selection
    color: colors.text
    placeholderTextColor: colors.placeholder

    // 背景设置
    background: Rectangle {
        implicitHeight: control.implicitH
        implicitWidth: 200
        color: colors.background
        border.color: control.activeFocus ? colors.borderFocus : colors.border
        border.width: control.activeFocus ? 2 : 1
        radius: control.radiusPx

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
