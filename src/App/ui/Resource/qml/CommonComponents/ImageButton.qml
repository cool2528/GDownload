import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Button{
    id:control
    property url normalImage
    property url hoverImage
    property color backgroundColor:"transparent"
    // 可选：为图标着色（暗色主题下保证对比）
    property color tintColor: "transparent"
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
        layer.enabled: tintColor !== "transparent"
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: control.tintColor
        }
    }
    HoverHandler{
        id:mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
