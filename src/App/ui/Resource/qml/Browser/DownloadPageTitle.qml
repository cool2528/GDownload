import QtQuick
import gdl.sdk 1.0
import QtQuick.Layouts
import "../CommonComponents"
Item {
    property int type: 0
    anchors.left: parent.left
    anchors.leftMargin: 20
    anchors.right: parent.right
    anchors.rightMargin: 20
    height:50
    Text {
        id: tipText
        anchors.verticalCenter: parent.verticalCenter
        text: {
            if(type === 0){
                return qsTr("Downloading")
            }else if(type === 1){
                return qsTr("Waiting")
            }else{
                return qsTr("Stopped")
            }
        }
        font.pixelSize: 14
        color: GTheme.dark ? "#ffffff" :  "#3b3b3b"
    }

    RowLayout{
        id:buttons
        anchors.right: parent.right
        anchors.top: tipText.top
        spacing: 20
        IconButton{
            id:delButton
            Layout.preferredWidth: 20
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.Delete
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                if(type ===0){
                    BrowserManager.RemoveAllTask(0,true)
                }else if(type ===1){
                    BrowserManager.RemoveAllTask(1,true)
                }else if(type === 2){
                    BrowserManager.RemoveAllTask(2,true)
                }
            }
        }
        IconButton{
            id:refreshButton
            Layout.preferredWidth: 20
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.Refresh
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                if(type ===0){
                    BrowserManager.RefreshTaskList(0)
                }else if(type ===1){
                    BrowserManager.RefreshTaskList(1)
                }else if(type === 2){
                    BrowserManager.RefreshTaskList(2)
                }
            }
        }
        IconButton{
            id:recoveryButton
            Layout.preferredWidth: 20
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            visible: type !== 2
            iconSource: SegoeFluentIcons.PlayBadge12
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                if(type ===0){
                    BrowserManager.UnpauseAllTask(0)
                }else if(type ===1){
                    BrowserManager.UnpauseAllTask(1)
                }else if(type === 2){
                    BrowserManager.UnpauseAllTask(2)
                }
            }
        }
        IconButton{
            id:pauseButton
            Layout.preferredWidth: 20
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            visible: type !== 2
            iconSource: SegoeFluentIcons.PauseBadge12
            iconColor:GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                if(type ===0){
                    BrowserManager.PauseAllTask(0)
                }else if(type ===1){
                    BrowserManager.PauseAllTask(1)
                }else if(type === 2){
                    BrowserManager.PauseAllTask(2)
                }
            }
        }
    }
    Rectangle{
        id:splitLine
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        width: parent.width
        height: 3
        color: GTheme.dark ? "#434343" : "#e5e5e5"
    }
}
