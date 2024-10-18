import QtQuick

Item {
    id:control
    property int iconSource
    property size iconSize
    Text{
        id:icon
        text: String.fromCharCode(iconSource).toString(16)
        width: iconSize.width
        height: iconSize.height
        font.family: SegoeFluentIcons
    }
}
