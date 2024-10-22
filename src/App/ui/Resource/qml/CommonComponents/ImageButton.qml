import QtQuick
import QtQuick.Controls.Basic

Button{
    id:control
    property url normalImage
    property url hoverImage
    property color backgroundColor:"transparent"
    property size imageSize: Qt.size(width,height)
    width: 30
    height: 30
    background: Rectangle{
        color:backgroundColor
    }
    Image {
        id:image
        source: mouse.hovered ? hoverImage : normalImage
        anchors.centerIn: parent
        width: imageSize.width
        height: imageSize.height
        mipmap: true
        smooth: true
        HoverHandler{
            id:mouse
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
    }
}
