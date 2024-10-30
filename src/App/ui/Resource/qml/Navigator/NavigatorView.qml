import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import fluentIcons 1.0
import QtQuick.Layouts
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
        Rectangle{
            id:downloadNavigator
            color: "#f2f3f6"
            implicitWidth: 200
            SplitView.minimumWidth: 200
        }
    }

}
