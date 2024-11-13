import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
CheckBox {
    id: control
    text: qsTr("Show download progress bar")
    checked: false

    indicator: Rectangle {
        implicitWidth: 15
        implicitHeight: 15
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 3
        border.color: control.hovered || control.down ? "#5f5ff9" : GTheme.dark ? "transparent" : "#dadde4"

        Rectangle {
            width: parent.width - 2
            height: parent.height -2
            color: control.checked ? "#5f5ff9" : "transparent"
            anchors.centerIn: parent
            visible: control.checked
            radius: 2
            Text {
                anchors.centerIn: parent
                text: "✓"
                color: "white"
                font.pixelSize: 14
                font.bold: true
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: GTheme.dark ? "#ffffff" :"#55575b"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
    HoverHandler{
        id:mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
