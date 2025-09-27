import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import gdl.sdk

Button {
    id: button
    property alias imageSource: image.source
    property alias tipText: text.text
    clip: true
    background: Rectangle{
        color: "transparent"
    }
    contentItem: Item {
        id: content
        anchors.fill: parent

        // 添加鼠标手状态
        HoverHandler {
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
        Rectangle{
            id:imageRect
            implicitHeight: 40
            implicitWidth: 70
            radius: 10
            border.width: button.checked ? 1 : 0
            border.color: GTheme.primaryColor
            color: "transparent"
            clip: true
            anchors{
                left: parent.left
                right: parent.right
                top: parent.top
            }
            Image {
                id: image
                anchors.fill: parent
                anchors.margins: 1
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: image.width
                        height: image.height
                        radius: imageRect.radius
                    }
                }
            }
        }
        Text {
            id: text
            text: button.text
            font.pixelSize: 12
            font.bold: button.checked
            anchors{
                top: imageRect.bottom
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                topMargin: 5
            }
            color: button.checked ? GTheme.primaryColor : GTheme.textPrimary

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
