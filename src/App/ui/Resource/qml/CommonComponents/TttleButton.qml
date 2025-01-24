import QtQuick
import QtQuick.Controls
import gdl.sdk 1.0
Button {
    id: control
    property int iconSource
    property int iconSize:control.font.pixelSize

    contentItem: Item {
        FontIcon{
            id:icon
            iconSize: control.iconSize
            iconSource: control.iconSource
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.top: parent.top
            anchors.topMargin: 7
            color: GTheme.dark ? (control.hovered || control.checked) ? "#ffffff" : "#a0a0a0" : (control.hovered || control.checked) ? "#5151f9" : "#494c55"
        }
        Text {
            id: text
            text: control.text
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: icon.right
            anchors.leftMargin: 20
            color: GTheme.dark ? (control.hovered || control.checked) ? "#ffffff" : "#a0a0a0" : (control.hovered || control.checked) ? "#5151f9" : "#494c55"
        }
        HoverHandler{
            id:mouse
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
    }

    background: Rectangle {
        color: GTheme.dark ? (control.hovered || control.checked) ? "#3c3c3c" : "transparent" : (control.hovered || control.checked) ? "#e7e9ee" : "transparent"
        radius: 2
    }
}
