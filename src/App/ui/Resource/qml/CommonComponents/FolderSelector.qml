import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import gdl.sdk
Item {
    id:folderSelector
    TextField {
        id: textField
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width - 30
        readOnly: !editable
        padding: 0
        leftPadding: 10
        color: "#DBDBDB"
        text: path
        selectByMouse: true
        selectionColor: "#3078BB"
        font.pixelSize: 14
        background: Rectangle {
            color: GTheme.dark ? parent.hovered ? Qt.lighter("#2E2E2E") : "#2E2E2E" : parent.hovered ? Qt.lighter("#ffffff") : "#ffffff"
            border.color: GTheme.dark ? parent.hovered ? "#5151f9" :"#545454" : parent.hovered ? "#5151f9" :"#a9a9a9"
            radius: 2
        }
        onEditingFinished: {
            if (text !== path) {
                let tmp = text
                text = Qt.binding(function(){return path})
                parent.textChanged(tmp)
            }
        }
        onActiveFocusChanged: {
            if (activeFocus) {
                selectAll()
                parent.actived()
            }
        }
    }
    Button{
        id:selectorBtn
        anchors{
            left: textField.right
            leftMargin: 5
            top: textField.top
        }
        width: 30
        height: 30
        background: Rectangle{
            color: GTheme.dark ? "#2d2d2d" : "#f3f6f9"
            border.color: GTheme.dark ? selectorBtn.activeFocus ? "#5151f9" : "#545454" : selectorBtn.activeFocus ? "#5151f9" : "#d7dae2"
        }
        Text{
            id:icon
            anchors{
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: -5
            }

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "..."
            color: GTheme.dark ? "#c4c4c4" : "#8d9096"
            font.pixelSize: 15
        }

        onHoveredChanged: {
            if(hovered){
                icon.color =  "#5151f9"
            }else{
                icon.color = GTheme.dark ? "#c4c4c4" : "#8d9096"
            }
        }
    }
}
