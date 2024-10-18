import QtQuick
import QtQuick.Controls

Item{
    id:control
    property int iconSource
    property size iconSize
    property color backgroundColor:backgroundRect.color
    Button{
        id:btn
        anchors.fill: parent
        contentItem: FontIcon{
            iconSize: iconSize
            iconSource: iconSource
        }
        background: Rectangle{
            id:backgroundRect
        }
    }
}
