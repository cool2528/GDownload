import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
Button {
    id:control
    property int type: 0 // 0 Default 1 primary
    background: Rectangle{
        radius: 2
        color:{
            if(GTheme.dark){
                if(type === 0){
                    return control.hovered ? "#2d2d2d" : "#515151"
                }else if(type === 1){
                    return control.hovered ? "#7171fa" : "#5151f9"
                 }
            }else{
                if(type === 0){
                    return control.hovered ? "#ededff" : "#ffffff"
                }else if(type === 1){
                    return control.hovered ? "#7171fa" : "#5151f9"
                 }
            }
        }
        border.color: GTheme.dark ? control.hovered ? "#555555" : "#515151" : control.hovered ? "#c7c7fe" : "#d9dbe3"
    }

    contentItem: Text {
        id: name
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font.pixelSize: 14
        color: {
            if(GTheme.dark){
                if(type === 0){
                    return control.hovered ? "#5151f9" : "#ffffff"
                }else if(type === 1){
                    return "#ffffff"
                 }
            }else{
                if(type === 0){
                    return control.hovered ? "#5151f9" : "#55575b"
                }else if(type === 1){
                    return "#ffffff"
                 }
            }
        }
    }
    HoverHandler{
        id:mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
