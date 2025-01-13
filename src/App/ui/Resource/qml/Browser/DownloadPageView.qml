import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
Item {
    id:control
    RowLayout{
        id:browserLayout
        anchors.fill: parent
        spacing: 0
        Rectangle{
            id:leftMenuBar
            color: GTheme.dark ? "#282828" :"#f2f3f6"
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            Text {
                id: title
                text: qsTr("Task")
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.top: parent.top
                anchors.topMargin: 30
                font.pixelSize: 14
                color: GTheme.dark ? "#ffffff" : "#3b3b3b"

            }
            ColumnLayout{
                id:bar
                anchors.top: title.bottom
                anchors.topMargin: 5
                width: parent.width
                property int currentIndex: 0
                property var buttonsArr: [download,waiting,stopped]
                ButtonGroup{
                    id:titleGroup
                    onCheckedButtonChanged: {
                        let index  = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                        bar.currentIndex = index
                        console.debug("index ",index)
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
                //Stopped
                TttleButton{
                    id:stopped
                    checkable: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.Stop
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Stopped")
                    onClicked: {
                        checked = true
                    }

                }
            }

        }

        Rectangle{
            id:browser
            color: GTheme.dark ? "#2e2e2e" : "#ffffff"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 380
            Layout.preferredWidth: 380
            DownloadPageTitle{
                id:downloadTitle
                type: bar.currentIndex

            }
            StackLayout{
                id:downloadStack
                anchors.top: downloadTitle.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                currentIndex: bar.currentIndex

                GDownloadViewPage {
                    id: downloadPage
                    pageType: 0
                    objectName: "downloadPage"
                    model: BrowserManager.GetActiveDownloadModel()
                }
                
                GDownloadViewPage {
                    id: waitingPage
                    pageType: 1
                    objectName: "waitingPage"
                    model: BrowserManager.GetWaitingDownloadModel()
                }
                
                GDownloadViewPage {
                    id: completedPage
                    pageType: 2
                    objectName: "completedPage"
                    model: BrowserManager.GetStopedDownloadModel()
                }
            }
        }
    }
}
