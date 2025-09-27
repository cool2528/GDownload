import QtQuick
import QtQuick.Controls
import gdl.sdk 1.0
Button {
    id: control
    property int iconSource
    property int iconSize: control.font.pixelSize

    contentItem: Item {
        Rectangle {
            id: selectedBar
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 2
            visible: control.checked
            color: GTheme.primaryColor
        }
        FontIcon{
            id:icon
            iconSize: control.iconSize
            iconSource: control.iconSource
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 7
            color: (control.hovered || control.checked) ? GTheme.primaryColor : (GTheme.dark ? GTheme.textSecondary : GTheme.textRegular)
        }
        Text {
            id: text
            text: control.text
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: icon.right
            anchors.leftMargin: 18
            color: (control.hovered || control.checked) ? GTheme.primaryColor : (GTheme.dark ? GTheme.textSecondary : GTheme.textRegular)
        }
        HoverHandler{
            id:mouse
            acceptedDevices: PointerDevice.Mouse
            cursorShape: Qt.PointingHandCursor
        }
    }

    background: Rectangle {
        color: (control.hovered || control.checked) ? GTheme.fillLight : "transparent"
        radius: 2
    }
}
