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
        color: GTheme.dark ? "#ffffff" : tipText.color
    }

    RowLayout{
        id:buttons
        anchors.right: parent.right
        anchors.top: tipText.top
        spacing: 20
        IconButton{
            id:delButton
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.Delete
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                console.debug("delete all task")
            }
        }
        IconButton{
            id:refreshButton
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.Refresh
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                console.debug("refresh all task")
            }
        }
        IconButton{
            id:recoveryButton
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.PlayBadge12
            iconColor: GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                console.debug("recovery all task")
            }
        }
        IconButton{
            id:pauseButton
            Layout.fillWidth: true
            Layout.minimumHeight: 40
            Layout.maximumHeight: 40
            iconSource: SegoeFluentIcons.PauseBadge12
            iconColor:GTheme.dark ? hovered ? "#5151f9" : "#ffffff": hovered ? "#5151f9" : "#8a8c91"
            onClicked: {
                console.debug("pause all task")
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
