import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import fluentIcons 1.0
Item {
    id:control
    RowLayout{
        id:browserLayout
        anchors.fill: parent
        Rectangle{
            id:leftMenuBar
            color: "#f2f3f6"
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            Text {
                id: title
                text: qsTr("Task List")
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.top: parent.top
                anchors.topMargin: 30
                font.pixelSize: 14

            }
            ColumnLayout{
                id:bar
                anchors.top: title.bottom
                anchors.topMargin: 5
                width: parent.width
                property int currentIndex: 0
                property var buttonsArr: [download,waiting,completed]
                ButtonGroup{
                    id:titleGroup
                    onCheckedButtonChanged: {
                        bar.currentIndex = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                    }
                }
                TttleButton{
                    id:download
                    checkable: true
                    checked: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.PlaySolid
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Downloading")
                    onClicked: {
                        checked = true
                    }
                }
                //Waiting
                TttleButton{
                    id:waiting
                    checkable: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.PauseBold
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Waiting")
                    onClicked: {
                        checked = true
                    }
                }
                //Completed
                TttleButton{
                    id:completed
                    checkable: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.CompletedSolid
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Completed")
                    onClicked: {
                        checked = true
                    }

                }
            }

        }

        Rectangle{
            id:browser
            color: "#ffffff"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 380
            Layout.preferredWidth: 380
            StackLayout{
                id:downloadStack
                anchors.fill: parent
                currentIndex: bar.currentIndex
                Rectangle{
                    id:downloadPage
                    color: "red"
                }
                Rectangle{
                    id:waitingPage
                    color: "green"
                }
                Rectangle{
                    id:completedPage
                    color: "blue"
                }
            }
        }
    }
}
