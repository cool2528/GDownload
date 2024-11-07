import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
SpinBox {
    id: control
    value: 50
    editable: true
    LayoutMirroring.enabled:false
    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        focus: true
        font: control.font
        color: GTheme.dark ? "#ffffff" :  "#7b7d80"
        selectionColor: GTheme.dark ? "" : "#acd2fe"
        selectedTextColor: GTheme.dark ? "#ffffff" : "#55575b"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    up.indicator: Rectangle {
        x: parent.width - width -2
        y:2
        height: parent.height/2 - 2
        width: height
        color: control.up.pressed ? "#f3f6f9" : "#f6f6f6"
        border.color: "#d7dae2"

        Text {
            text: "▲"
            font.pixelSize: control.font.pixelSize * 2
            color: GTheme.dark ? "#9e9e9e" :"#94969a"
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    down.indicator: Rectangle {
        x: parent.width - width -2
        y: parent.height - height -2
        height: parent.height/2 -2
        width: height
        color: control.down.pressed ? "#e4e4e4" : "#f6f6f6"
        border.color: "#d7dae2"

        Text {
            text: "▼"
            font.pixelSize: control.font.pixelSize * 2
            color: GTheme.dark ? "#9e9e9e" :"#94969a"
            anchors.fill: parent
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 30
        color: GTheme.dark ? "#303030" : "#ffffff"
        border.color: control.focus ? "#5151f9" : "#d7dae2"
    }
}
