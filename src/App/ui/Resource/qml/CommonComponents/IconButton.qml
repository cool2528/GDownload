import QtQuick
import QtQuick.Controls
Button{
    id:control
    property int iconSource
    property int iconSize
    property color backgroundColor:"transparent"
    property alias iconColor: icon.color
    implicitHeight: {
        return textMetrics.height
    }
    implicitWidth: {
        return textMetrics.width
    }

    background: Rectangle{
        color: control.backgroundColor
    }
    FontIcon{
        id:icon
        anchors.fill: parent
        iconSize: control.iconSize
        iconSource: control.iconSource
        color:icon.color
    }
    HoverHandler{
        id:mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
    TextMetrics{
        id:textMetrics
        font.family: FluentIcons
        font.pixelSize: iconSize
        text: String.fromCharCode(iconSource).toString(16)
    }

}
