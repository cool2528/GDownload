import QtQuick
import QtQuick.Controls
import gdl.sdk

Button {
    id: control
    
    // 属性定义
    // 向后兼容：0=default,1=primary
    property int type: 0
    // 新增规格化 API（不破坏旧用法）
    // buttonType: default | primary | success | warning | danger | info
    property string buttonType: ""    // 若为空则根据旧的 numeric type 推断
    // variant: default | plain | round | circle | link
    property string variant: "default"
    // size: large | default | small
    property string size: "default"
    property real radius: 2
    // 如未设置，自适应 size；若设置则尊重自定义
    property real fontSize: 0

    // 解析最终类型（兼容旧的 numeric type）
    readonly property string resolvedType: (function(){
        if (buttonType && buttonType.length>0) return buttonType.toLowerCase();
        return type === 1 ? "primary" : "default";
    })()

    // 尺寸规格
    readonly property int implicitH: (size === "large" ? 40 : (size === "small" ? 24 : 32))
    readonly property int hPadding: (size === "large" ? 16 : (size === "small" ? 8 : 12))
    readonly property int fontPx: (fontSize > 0 ? fontSize : (size === "large" ? 16 : (size === "small" ? 12 : 14)))
    
    // 内部属性用于颜色管理（基于 GTheme/ElementPlusColors 收敛）
    readonly property var colors: {
        const darkTheme = {
            default: {
                bg: GTheme.fillBase,
                bgHover: GTheme.fillLight,
                bgChecked: GTheme.fillLight,
                text: GTheme.textRegular,
                textHover: GTheme.primaryColor,
                textChecked: GTheme.primaryColor,
                border: GTheme.borderBase,
                borderHover: GTheme.borderLight,
                borderChecked: GTheme.primaryColor
            },
            primary: {
                bg: GTheme.primaryColor,
                bgHover: GTheme.primaryLight(3),
                bgChecked: GTheme.primaryLight(4),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            success: {
                bg: GTheme.successColor,
                bgHover: GTheme.successLight(7),
                bgChecked: GTheme.successLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            warning: {
                bg: GTheme.warningColor,
                bgHover: GTheme.warningLight(7),
                bgChecked: GTheme.warningLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            danger: {
                bg: GTheme.dangerColor,
                bgHover: GTheme.dangerLight(7),
                bgChecked: GTheme.dangerLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            info: {
                bg: GTheme.infoColor,
                bgHover: GTheme.infoLight(7),
                bgChecked: GTheme.infoLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        }

        const lightTheme = {
            default: {
                bg: GTheme.bgWhite,
                bgHover: GTheme.fillLight,
                bgChecked: GTheme.fillLight,
                text: GTheme.textRegular,
                textHover: GTheme.primaryColor,
                textChecked: GTheme.primaryColor,
                border: GTheme.borderBase,
                borderHover: GTheme.borderLight,
                borderChecked: GTheme.primaryColor
            },
            primary: {
                bg: GTheme.primaryColor,
                bgHover: GTheme.primaryLight(2),
                bgChecked: GTheme.primaryLight(3),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            success: {
                bg: GTheme.successColor,
                bgHover: GTheme.successLight(7),
                bgChecked: GTheme.successLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            warning: {
                bg: GTheme.warningColor,
                bgHover: GTheme.warningLight(7),
                bgChecked: GTheme.warningLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            danger: {
                bg: GTheme.dangerColor,
                bgHover: GTheme.dangerLight(7),
                bgChecked: GTheme.dangerLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            info: {
                bg: GTheme.infoColor,
                bgHover: GTheme.infoLight(7),
                bgChecked: GTheme.infoLight(6),
                text: ElementPlusColors.bgWhite(false),
                textHover: ElementPlusColors.bgWhite(false),
                textChecked: ElementPlusColors.bgWhite(false),
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        }

        return GTheme.dark ? darkTheme : lightTheme
    }
    
    // 获取当前主题色（增加兜底，避免未初始化时出现 undefined）
    readonly property var currentTheme: (function(){
        const theme = colors;
        const key = (resolvedType in theme) ? resolvedType : (type === 0 ? "default" : "primary");
        if (theme && theme[key]) return theme[key];
        // fallback
        return {
            bg: GTheme.bgWhite,
            bgHover: GTheme.fillLight,
            bgChecked: GTheme.fillLight,
            text: GTheme.textRegular,
            textHover: GTheme.textRegular,
            textChecked: GTheme.textRegular,
            border: GTheme.borderBase,
            borderHover: GTheme.borderLight,
            borderChecked: GTheme.borderBase
        }
    })()

    background: Rectangle {
        radius: (variant === "round" || variant === "circle") ? 20 : control.radius
        implicitHeight: control.implicitH
        implicitWidth: Math.max(implicitHeight, textItem.implicitWidth + control.hPadding * 2)
        color: {
            if (!control.enabled) {
                return GTheme.dark ? GTheme.fillBase : GTheme.fillLight
            }
            if (variant === "plain" || variant === "link") {
                return control.hovered ? (GTheme.dark ? GTheme.fillBase : GTheme.fillLight) : "transparent"
            }
            if (control.checked) {
                return currentTheme.bgChecked
            }
            return control.hovered ? currentTheme.bgHover : currentTheme.bg
        }
        border.color: {
            if (!control.enabled) {
                return GTheme.dark ? GTheme.borderBase : GTheme.borderLight
            }
            if (variant === "plain") {
                return currentTheme.textHover
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
        font.pixelSize: control.fontPx
        id: textItem
        color: {
            if (!control.enabled) {
                return GTheme.textDisabled
            }
            if (variant === "link") {
                return control.hovered ? currentTheme.textHover : currentTheme.text
            }
            if (control.checked || variant === "plain") {
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

    // 圆形按钮宽高一致
    Component.onCompleted: {
        if (variant === "circle") {
            width = background.implicitHeight
        }
    }
}
