import QtQuick
import QtQuick.Controls
import gdl.sdk

Button {
    id: control
    property int iconSource
    property int iconSize: 16
    property color backgroundColor: "transparent"
    property alias iconColor: icon.color

    implicitHeight: 32
    implicitWidth: 32

    background: Rectangle {
        color: control.backgroundColor
        radius: 4
    }

    contentItem: FontIcon {
        id: icon
        anchors.centerIn: parent
        iconSize: control.iconSize
        iconSource: control.iconSource
        color: GTheme.textSecondary
    }

    HoverHandler {
        id: mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
