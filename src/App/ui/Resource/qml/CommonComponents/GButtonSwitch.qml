import QtQuick
import QtQuick.Controls
import gdl.sdk
Switch {
    id: control
    // 规格化尺寸：large/default/small
    property string size: "default"
    readonly property int trackH: (size === "large" ? 28 : (size === "small" ? 18 : 22))
    readonly property int trackW: Math.round(trackH * 1.8)
    readonly property int knob: trackH - 2
    property color checkedBkColor: GTheme.primaryColor
    property color normalBkColor: GTheme.borderBase
    property color checkedFkColor: GTheme.bgWhite
    property color normalFkColor: GTheme.bgWhite
    property color textColor: GTheme.textPrimary
    implicitHeight: trackH
    font.pixelSize: 14
    indicator: Rectangle {
        implicitWidth: control.trackW
        implicitHeight: control.trackH
        x: control.width - width - control.rightPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: control.checked ? checkedBkColor : normalBkColor
        border.color: control.checked ? checkedBkColor : normalBkColor

        // 添加背景颜色过渡动画
        Behavior on color {
            ColorAnimation { duration: 200 }
        }
        Behavior on border.color {
            ColorAnimation { duration: 200 }
        }

        Rectangle {
            id: toggleButton
            x: control.checked ? parent.width - width - 1 : 1
            y: parent.height / 2 - height / 2
            width: control.knob
            height: control.knob
            radius: height / 2
            color: control.checked ? checkedFkColor : normalFkColor
            border.color: control.checked ? checkedFkColor : normalFkColor

            // 添加滑块移动动画
            Behavior on x {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.InOutQuad
                }
            }
            // 添加滑块颜色过渡动画
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
            Behavior on border.color {
                ColorAnimation { duration: 200 }
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: textColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        rightPadding: control.indicator.width + control.spacing
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }
}
