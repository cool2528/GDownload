import QtQuick
import QtQuick.Controls.Basic
Button{
    id:control
    property int iconSource
    property int iconSize
    property color backgroundColor:"transparent"
    property alias iconColor: icon.color
    background: Rectangle{
        id:backgroundRect
        color: control.backgroundColor
    }
    FontIcon{
        id:icon
        anchors.fill: parent
        iconSize: control.iconSize
        iconSource: control.iconSource
        HoverHandler{
            id:mouse
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
    }
}
