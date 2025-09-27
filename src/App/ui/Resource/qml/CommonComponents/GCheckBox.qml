import QtQuick
import QtQuick.Controls
import gdl.sdk

CheckBox {
    id: control
    checked: false
    // Element Plus 规格化 API
    // size: large | default | small
    property string size: "default"
    // border: 添加边框样式（Element Plus 特性）
    property bool border: false
    // status: normal | success | warning | danger（扩展状态支持）
    property string status: "normal"

    readonly property int indicatorPx: (size === "large" ? 18 : (size === "small" ? 14 : 16))
    readonly property int fontPx: (size === "large" ? 16 : (size === "small" ? 12 : 14))
    readonly property int borderRadius: 3
    
    // Element Plus 风格颜色管理
    readonly property var colors: {
        const baseColors = {
            primary: {
                background: GTheme.bgWhite,
                backgroundChecked: GTheme.primaryColor,
                border: GTheme.borderBase,
                borderHover: GTheme.primaryColor,
                borderChecked: GTheme.primaryColor,
                checkmark: GTheme.bgWhite,
                text: GTheme.textRegular
            },
            success: {
                background: GTheme.bgWhite,
                backgroundChecked: GTheme.successColor,
                border: GTheme.borderBase,
                borderHover: GTheme.successColor,
                borderChecked: GTheme.successColor,
                checkmark: GTheme.bgWhite,
                text: GTheme.textRegular
            },
            warning: {
                background: GTheme.bgWhite,
                backgroundChecked: GTheme.warningColor,
                border: GTheme.borderBase,
                borderHover: GTheme.warningColor,
                borderChecked: GTheme.warningColor,
                checkmark: GTheme.bgWhite,
                text: GTheme.textRegular
            },
            danger: {
                background: GTheme.bgWhite,
                backgroundChecked: GTheme.dangerColor,
                border: GTheme.borderBase,
                borderHover: GTheme.dangerColor,
                borderChecked: GTheme.dangerColor,
                checkmark: GTheme.bgWhite,
                text: GTheme.textRegular
            }
        }

        // 暗色主题适配
        if (GTheme.dark) {
            const darkColors = JSON.parse(JSON.stringify(baseColors))
            Object.keys(darkColors).forEach(key => {
                darkColors[key].background = GTheme.fillBase
                darkColors[key].text = GTheme.textPrimary
            })
            return darkColors
        }

        return baseColors
    }

    // 当前状态颜色
    readonly property var currentColors: {
        const statusKey = (status === "success" || status === "warning" || status === "danger") ? status : "primary"
        return colors[statusKey] || colors.primary
    }

    indicator: Rectangle {
        implicitWidth: control.indicatorPx
        implicitHeight: control.indicatorPx
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: control.borderRadius
        color: control.checked ? currentColors.backgroundChecked : currentColors.background
        border.color: {
            if (!control.enabled) {
                return GTheme.dark ? GTheme.borderBase : GTheme.borderLight
            }
            if (control.checked) {
                return currentColors.borderChecked
            }
            return control.hovered ? currentColors.borderHover : currentColors.border
        }
        border.width: (control.hovered || control.checked) ? 1.5 : 1
        
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

                    // Element Plus 风格勾选标记
                    ctx.strokeStyle = currentColors.checkmark
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"

                    // 优化的 Element Plus 勾选路径
                    var w = width
                    var h = height
                    var padding = w * 0.15  // 更贴近 Element Plus 的边距

                    ctx.beginPath()
                    ctx.moveTo(padding + 1, h * 0.5)
                    ctx.lineTo(w * 0.42, h * 0.72)
                    ctx.lineTo(w - padding - 1, h * 0.28)
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
        font.pixelSize: control.fontPx
        opacity: enabled ? 1.0 : 0.3
        color: currentColors.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing

        // Element Plus 边框样式支持
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            color: "transparent"
            border.color: control.border ? currentColors.border : "transparent"
            border.width: control.border ? 1 : 0
            radius: control.borderRadius
            visible: control.border

            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    // 使用 HoverHandler 设置鼠标形状
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
