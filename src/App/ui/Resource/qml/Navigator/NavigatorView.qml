import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import fluentIcons 1.0
Item{
    id:navigator

    SplitView{
        id:navigatorSplitView
        anchors.fill: parent
        handle: Rectangle{
            id:handleDelegate
            implicitWidth: 0
            color: "transparent"
        }
        Rectangle{
            id:systemNavigator
            color: "#484848"
            implicitWidth: 74
            SplitView.minimumWidth: 74
            IconButton{
                id:home
                width: 30
                height: 30
                iconSource: SegoeFluentIcons.HomeSolid
                iconSize: 30
                iconColor: "#ffffff"
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.left: parent.left
                anchors.leftMargin: 20
            }
        }
        Rectangle{
            id:downloadNavigator
            color: "#f2f3f6"
            implicitWidth: 200
            SplitView.minimumWidth: 200
        }
    }

}
