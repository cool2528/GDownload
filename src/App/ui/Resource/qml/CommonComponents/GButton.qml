import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

// 规范按钮基底:吸收纯文字按钮 / 图标按钮(icon-only)/ 导航项(variant:"nav")
// 颜色/尺寸/间距/动效一律取自 GTheme 令牌,调用方不再传入颜色
Button {
    id: control

    activeFocusOnTab: enabled && visible
    Accessible.name: control.text

    // ===== 向后兼容 API(勿删,59 处调用依赖)=====
    // 0=default,1=primary
    property int type: 0
    // buttonType: default | primary | success | warning | danger | info(优先于 numeric type)
    property string buttonType: ""
    // variant: default | plain | round | circle | link | nav | chip
    property string variant: "default"
    // size: large | default | small
    property string size: "default"
    // 消费级默认按钮圆角(V5 设计稿,原 radiusSmall=2 过于尖锐)
    property real radius: GTheme.radiusLarge
    // 如未设置则按 size 自适应
    property real fontSize: 0

    // ===== 新增内容槽 =====
    // FontIcon 字形(>0 视为有图标)
    property int iconSource: 0
    property int iconSize: GTheme.fontBody
    // 哨兵 transparent:为空则自动跟随文字色/类型色,调用方一般无需设置
    property color iconColor: "transparent"
    // Aurora 语义 SVG 图标名；优先于 iconSource/imageSource。
    property string iconName: ""
    // 图片图标(与 FontIcon 二选一,用于 SVG 等非字体图标)
    property url imageSource: ""
    property url hoverImage: ""
    property color tintColor: "transparent"
    property size imageSize: Qt.size(iconSize, iconSize)

    // ===== 派生状态 =====
    readonly property bool isNav: variant === "nav"
    readonly property bool isChip: variant === "chip"
    readonly property bool hasSemanticIcon: iconName.length > 0
    readonly property bool hasIcon: iconSource > 0
    readonly property bool hasImage: imageSource.toString().length > 0
    readonly property bool hasText: control.text !== ""
    // 纯图标态:有图标/图片但无文字,且非导航
    readonly property bool iconOnly: (hasSemanticIcon || hasIcon || hasImage) && !hasText && !isNav

    readonly property string resolvedType: (function () {
        if (buttonType && buttonType.length > 0)
            return buttonType.toLowerCase();
        return type === 1 ? "primary" : "default";
    })()

    // 导航选中指示条宽度(Element Plus 设计常量)
    readonly property int navIndicatorWidth: 3
    // 彩色按钮上的文字色恒为白(不随主题),不走 ElementPlusColors 直调
    readonly property color onAccentText: GTheme.textInverse

    // 尺寸规格(令牌)
    readonly property int implicitH: isNav ? GTheme.navItemHeight : (isChip ? GTheme.sizeSmall : (size === "large" ? GTheme.sizeLarge : (size === "small" ? GTheme.sizeSmall : GTheme.sizeDefault)))
    readonly property int hPadding: isChip ? GTheme.spaceSM : (size === "large" ? GTheme.spaceLG : (size === "small" ? GTheme.spaceSM : GTheme.spaceMD))
    readonly property int fontPx: fontSize > 0 ? fontSize : (isChip ? GTheme.fontCaption : (size === "large" ? GTheme.fontSubtitle : (size === "small" ? GTheme.fontCaption : GTheme.fontBody)))

    padding: 0

    // 类型主色(用于 icon-only 悬停态)
    function typeMainColor() {
        switch (resolvedType) {
        case "success":
            return GTheme.successColor;
        case "warning":
            return GTheme.warningColor;
        case "danger":
            return GTheme.dangerColor;
        case "info":
            return GTheme.infoColor;
        default:
            return GTheme.primaryColor;
        }
    }

    // 颜色管理(基于 GTheme 收敛,白字用常量)
    readonly property var colors: {
        const darkTheme = {
            "default": {
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
            "primary": {
                bg: GTheme.primaryColor,
                bgHover: GTheme.brandHover,
                bgChecked: GTheme.brandPressed,
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "success": {
                bg: GTheme.successColor,
                bgHover: GTheme.successLight(3),
                bgChecked: GTheme.successLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "warning": {
                bg: GTheme.warningColor,
                bgHover: GTheme.warningLight(3),
                bgChecked: GTheme.warningLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "danger": {
                bg: GTheme.dangerColor,
                bgHover: GTheme.dangerLight(3),
                bgChecked: GTheme.dangerLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "info": {
                bg: GTheme.infoColor,
                bgHover: GTheme.infoLight(3),
                bgChecked: GTheme.infoLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        };

        const lightTheme = {
            "default": {
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
            "primary": {
                bg: GTheme.primaryColor,
                bgHover: GTheme.brandHover,
                bgChecked: GTheme.brandPressed,
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "success": {
                bg: GTheme.successColor,
                bgHover: GTheme.successLight(3),
                bgChecked: GTheme.successLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "warning": {
                bg: GTheme.warningColor,
                bgHover: GTheme.warningLight(3),
                bgChecked: GTheme.warningLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "danger": {
                bg: GTheme.dangerColor,
                bgHover: GTheme.dangerLight(3),
                bgChecked: GTheme.dangerLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            },
            "info": {
                bg: GTheme.infoColor,
                bgHover: GTheme.infoLight(3),
                bgChecked: GTheme.infoLight(5),
                text: control.onAccentText,
                textHover: control.onAccentText,
                textChecked: control.onAccentText,
                border: "transparent",
                borderHover: "transparent",
                borderChecked: "transparent"
            }
        };

        return GTheme.dark ? darkTheme : lightTheme;
    }

    // 当前主题色(带兜底,避免未初始化 undefined)
    readonly property var currentTheme: (function () {
        const theme = colors;
        const key = (resolvedType in theme) ? resolvedType : (type === 0 ? "default" : "primary");
        if (theme && theme[key])
            return theme[key];
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
        };
    })()

    // 文字/图标统一前景色
    readonly property color contentColor: {
        if (!control.enabled)
            return GTheme.textDisabled;
        if (control.isNav)
            return (control.hovered || control.checked) ? GTheme.primaryColor : (GTheme.dark ? GTheme.textSecondary : GTheme.textRegular);
        if (control.iconOnly)
            return (control.hovered || control.checked) ? typeMainColor() : GTheme.textSecondary;
        if (variant === "link")
            return control.hovered ? currentTheme.textHover : currentTheme.text;
        if (control.checked || variant === "plain")
            return currentTheme.textChecked;
        return control.hovered ? currentTheme.textHover : currentTheme.text;
    }
    // 图标有效色:未显式设置则跟随 contentColor
    readonly property color effIconColor: Qt.colorEqual(iconColor, "transparent") ? contentColor : iconColor

    // 悬停填充色(nav/iconOnly/plain 复用);"无背景"态必须用同色 0-alpha 而非 "transparent":
    // transparent 的 RGB 是黑色,ColorAnimation 在 RGBA 上插值会途经半透明黑,
    // 浅色主题下表现为 hover 进出时深灰一闪
    readonly property color navActiveFill: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)
    readonly property color hoverFill: GTheme.dark ? GTheme.fillBase : GTheme.fillLight

    background: Rectangle {
        radius: (variant === "round" || variant === "circle") ? GTheme.radiusRound : (control.isNav ? GTheme.radiusBase : (control.isChip ? GTheme.radiusRound : control.radius))
        implicitHeight: control.implicitH
        implicitWidth: control.isNav ? (GTheme.spaceLG + control.iconSize + GTheme.spaceMD + navText.implicitWidth + GTheme.spaceLG) : (control.iconOnly ? control.implicitH : Math.max(implicitHeight, stdRow.implicitWidth + control.hPadding * 2))
        color: {
            if (control.isNav)
                return (control.hovered || control.checked) ? control.navActiveFill : Qt.alpha(control.navActiveFill, 0);
            if (control.iconOnly)
                return control.checked ? control.navActiveFill
                                       : (control.hovered ? control.hoverFill : Qt.alpha(control.hoverFill, 0));
            if (!control.enabled)
                return GTheme.dark ? GTheme.fillBase : GTheme.fillLight;
            if (variant === "plain" || variant === "link")
                return control.hovered ? control.hoverFill : Qt.alpha(control.hoverFill, 0);
            if (control.checked)
                return currentTheme.bgChecked;
            return control.hovered ? currentTheme.bgHover : currentTheme.bg;
        }
        border.color: {
            if (control.isNav || control.iconOnly)
                return "transparent";
            if (!control.enabled)
                return GTheme.dark ? GTheme.borderBase : GTheme.borderLight;
            if (variant === "plain")
                return currentTheme.textHover;
            if (control.checked)
                return currentTheme.borderChecked;
            return control.hovered ? currentTheme.borderHover : currentTheme.border;
        }
        border.width: (control.isNav || control.iconOnly) ? 0 : (control.checked ? 1.5 : 1)

        Behavior on color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }
        Behavior on border.width {
            NumberAnimation {
                duration: GTheme.durationBase
            }
        }
    }

    contentItem: Item {
        id: contentLayout
        implicitWidth: control.isNav ? (control.iconSize + GTheme.spaceMD + navText.implicitWidth) : stdRow.implicitWidth
        implicitHeight: control.implicitH

        // ===== 导航选中指示条 =====
        Rectangle {
            id: navIndicator
            visible: control.isNav
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: control.navIndicatorWidth
            height: parent.height * 0.5
            radius: width / 2
            color: GTheme.primaryColor
            opacity: control.checked ? 1 : 0
            scale: control.checked ? 1 : 0
            Behavior on scale {
                NumberAnimation {
                    duration: GTheme.durationSlow
                    easing.type: GTheme.easingStandard
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: GTheme.durationBase
                }
            }
        }

        // ===== 导航内容:左对齐 图标 + 文本 =====
        Item {
            id: navIcon
            visible: control.isNav
            width: control.iconSize
            height: control.iconSize
            anchors.left: parent.left
            anchors.leftMargin: GTheme.spaceLG
            anchors.verticalCenter: parent.verticalCenter

            AuroraIcon {
                anchors.centerIn: parent
                visible: control.hasSemanticIcon
                name: control.hasSemanticIcon ? control.iconName : "info"
                iconSize: control.iconSize
                color: control.effIconColor
            }

            FontIcon {
                anchors.centerIn: parent
                visible: !control.hasSemanticIcon && control.hasIcon
                iconSize: control.iconSize
                iconSource: control.iconSource
                color: control.effIconColor
            }
        }
        Text {
            id: navText
            visible: control.isNav
            text: control.text
            anchors.left: navIcon.right
            anchors.leftMargin: GTheme.spaceMD
            anchors.right: parent.right
            anchors.rightMargin: GTheme.spaceLG
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: control.fontPx
            font.weight: control.checked ? GTheme.weightMedium : GTheme.weightRegular
            color: control.contentColor
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        // ===== 标准内容:居中 图标/图片 + 文本 =====
        Row {
            id: stdRow
            visible: !control.isNav
            anchors.centerIn: parent
            spacing: (control.hasText && (control.hasSemanticIcon || control.hasIcon || control.hasImage)) ? GTheme.spaceSM : 0

            AuroraIcon {
                visible: control.hasSemanticIcon
                anchors.verticalCenter: parent.verticalCenter
                name: control.hasSemanticIcon ? control.iconName : "info"
                iconSize: control.iconSize
                color: control.effIconColor
            }

            FontIcon {
                visible: control.hasIcon && !control.hasSemanticIcon && !control.hasImage
                anchors.verticalCenter: parent.verticalCenter
                iconSize: control.iconSize
                iconSource: control.iconSource
                color: control.effIconColor
            }
            Image {
                visible: control.hasImage && !control.hasSemanticIcon
                anchors.verticalCenter: parent.verticalCenter
                source: (control.hoverImage.toString().length > 0 && control.hovered) ? control.hoverImage : control.imageSource
                width: control.imageSize.width
                height: control.imageSize.height
                mipmap: true
                smooth: true
                fillMode: Image.PreserveAspectFit
                layer.enabled: !Qt.colorEqual(control.tintColor, "transparent")
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: control.tintColor
                }
            }
            Text {
                visible: control.hasText
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font.pixelSize: control.fontPx
                color: control.contentColor

                Behavior on color {
                    ColorAnimation {
                        duration: GTheme.durationBase
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: (control.variant === "round" || control.variant === "circle")
                ? GTheme.radiusRound : control.radius + 2
        color: "transparent"
        border.width: 2
        border.color: GTheme.focusRing
        visible: control.activeFocus
        z: 2
    }

    // 鼠标指针形状
    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    // 按下反馈
    scale: control.pressed ? 0.98 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: GTheme.durationFast
            easing.type: GTheme.easingStandard
        }
    }

    // 圆形按钮宽高一致
    Component.onCompleted: {
        if (variant === "circle")
            width = background.implicitHeight;
    }
}
