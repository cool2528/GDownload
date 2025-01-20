import QtQuick
import QtQuick.Controls
import gdl.sdk
Switch {
    id: control
    property color checkedBkColor: "#1887EE"
    property color normalBkColor: "#bbbbbb"
    property color checkedFkColor: "#FFFFFF"
    property color normalFkColor: "#ffffff"
    property color textColor: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
    implicitHeight: 28
    font.pixelSize: 14
    indicator: Rectangle {
        implicitWidth: 40
        implicitHeight: 22
        x: control.width - width - control.rightPadding
        y: parent.height / 2 - height / 2
        radius: 11
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
            width: 20
            height: 20
            radius: 10
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
        rightPadding: control.indicator.width + control.spacing
    }
}
