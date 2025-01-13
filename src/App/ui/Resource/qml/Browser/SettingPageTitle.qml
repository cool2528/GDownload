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
                return qsTr("Basic")
            }else if(type === 1){
                return qsTr("Advanced")
            }else{
                return qsTr("Lab")
            }
        }
        font.pixelSize: 14
        color: GTheme.dark ? "#ffffff" : "#3b3b3b"
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
