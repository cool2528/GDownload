import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
TextField {
    id:control
    background: Rectangle{
        implicitHeight: 30
        implicitWidth: 200
        border.color: GTheme.dark ? control.activeFocus ? "#5151f9" : "#484848" : control.activeFocus ? "#5151f9" : "#d7dae2"
        color: GTheme.dark  ? "#303030" : "#ffffff"
    }
    color: GTheme.dark ? "#ffffff" : "#303133"
    placeholderTextColor: color
}
