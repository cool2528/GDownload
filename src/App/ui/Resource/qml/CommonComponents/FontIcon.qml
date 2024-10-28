import QtQuick

Item {
    id:control
    property int iconSource
    property int iconSize
    property alias color: icon.color
    Text{
        id:icon
        text: String.fromCharCode(iconSource).toString(16)
        font.pixelSize:iconSize
        font.family: FluentIcons
    }

}
