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
        Rectangle{
            id:imageRect
            implicitHeight: 40
            implicitWidth: 70
            radius: 10
            border.width: button.checked ? 1 : 0
            border.color: "#5b5bfa"
            color: "transparent"
            clip: true
            anchors{
                left: parent.left
                right: parent.right
                top: parent.top
            }
            Image {
                id:image
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
            color: {
                if(GTheme.dark){
                    return button.checked ? "#5b5bfa" : "#ffffff"
                }
                return button.checked ? "#5b5bfa" : "#3b3b3b"
            }

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
