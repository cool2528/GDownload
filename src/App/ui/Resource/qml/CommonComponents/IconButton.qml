import QtQuick
import QtQuick.Controls

Button{
    id:control
    property int iconSource
    property size iconSize
    property color backgroundColor:backgroundRect.color
    anchors.fill: parent
    contentItem: FontIcon{
        iconSize: iconSize
        iconSource: iconSource
    }
    background: Rectangle{
        id:backgroundRect
    }
}
