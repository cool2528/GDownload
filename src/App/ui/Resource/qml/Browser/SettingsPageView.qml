import QtQuick
import QtQuick.Layouts
Item {
    id:control
    RowLayout{
        id:browserLayout
        anchors.fill: parent
        Rectangle{
            id:leftMenuBar
            color: "red"
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
        }

        Rectangle{
            id:browser
            color: "blue"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 380
            Layout.preferredWidth: 380
        }
    }
}
