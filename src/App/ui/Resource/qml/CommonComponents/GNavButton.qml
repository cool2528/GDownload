import QtQuick
import QtQuick.Controls
import gdl.sdk

// Element Plus 风格导航按钮
Button {
    id: control
    property int iconSource
    property int iconSize: 16

    // 简化的颜色管理，避免复杂对象计算导致的闪烁

    contentItem: Item {
        // Element Plus 风格选中指示器
        Rectangle {
            id: selectedIndicator
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            radius: 1.5
            visible: control.checked
            color: GTheme.primaryColor

            // 选中指示器动画
            scale: control.checked ? 1.0 : 0.0
            Behavior on scale {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }
        }

        // Element Plus 风格图标
        FontIcon {
            id: icon
            width: 20  // 固定图标容器宽度
            height: 20 // 固定图标容器高度
            iconSize: control.iconSize
            iconSource: control.iconSource
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            color: (control.hovered || control.checked) ?
                      GTheme.primaryColor :
                      (GTheme.dark ? GTheme.textSecondary : GTheme.textRegular)
        }

        // Element Plus 风格文本标签
        Text {
            id: textLabel
            text: control.text
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: icon.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 16
            font.pixelSize: 14
            font.weight: control.checked ? Font.Medium : Font.Normal
            color: (control.hovered || control.checked) ?
                      GTheme.primaryColor :
                      (GTheme.dark ? GTheme.textSecondary : GTheme.textRegular)
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        color: (control.hovered || control.checked) ?
                  (GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)) :
                  "transparent"
        radius: 6  // Element Plus 导航项圆角
    }

    // 使用 HoverHandler 仅设置鼠标形状
    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }

    // 点击反馈效果
    scale: control.pressed ? 0.98 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }
}