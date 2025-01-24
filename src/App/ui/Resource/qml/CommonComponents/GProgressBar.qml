import QtQuick
import QtQuick.Controls
ProgressBar {
    id: control
    padding:0
    property color bkColor: "#ffffff"
    property color fgColor: "#5151e7"
    background: Rectangle {
        implicitWidth: control.width
        implicitHeight: control.height
        color: bkColor
        radius: control.height / 2
    }

    contentItem: Item {
        implicitWidth: control.width
        implicitHeight: control.height

        Rectangle {
            width: control.visualPosition * control.width
            height: control.height
            radius: height / 2
            color: fgColor
        }
    }
}
