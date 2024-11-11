import QtQuick

Item {
    id:control
    property int iconSource
    property int iconSize
    property alias color: icon.color
    property alias horizontalAlignment: icon.horizontalAlignment
    property alias verticalAlignment: icon.verticalAlignment
    Text{
        id:icon
        text: String.fromCharCode(iconSource).toString(16)
        font.pixelSize:iconSize
        font.family: FluentIcons
    }

}
