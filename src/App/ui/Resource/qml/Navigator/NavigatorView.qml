import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
import QtQuick.Layouts

Item{
    id:navigator
    Rectangle{
        id:systemNavigator
        anchors.fill: parent
        color: "#484848"
        implicitWidth: 74
        SplitView.minimumWidth: 74
        // topLayout
        ColumnLayout{
            id:topLayout
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.left: parent.left
            anchors.leftMargin: 20
            spacing: 10
            IconButton{
                id:home
                iconSource: SegoeFluentIcons.HomeSolid
                iconSize: 30
                iconColor: "#ffffff"
                onClicked: {
                    Qt.openUrlExternally("https://www.baidu.com/")
                }
            }
            IconButton{
                id:addTask
                Layout.topMargin: 40
                iconSource: SegoeFluentIcons.SubscriptionAdd
                iconSize: 30
                iconColor: "#ffffff"
                onClicked: {
                    console.log("open add task dialog")
                    brower_view.index = 0
                }
            }
        }
        //bottomLayout
        ColumnLayout{
            id:bottomLayout
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.left: parent.left
            anchors.leftMargin: 20
            spacing: 10
            IconButton{
                id:setting
                iconSource: SegoeFluentIcons.SettingsSolid
                iconSize: 30
                iconColor: "#ffffff"
                onClicked: {
                    console.log("open settings dialog")
                    brower_view.index = 1
                }
            }
            IconButton{
                id:help
                Layout.topMargin: 40
                iconSource: SegoeFluentIcons.Info
                iconSize: 30
                iconColor: "#ffffff"
                onClicked: {
                    console.log("open help dialog")

                }
            }
        }

    }

}
