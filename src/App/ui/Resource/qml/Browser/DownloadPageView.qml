import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
Item {
    id:control
    property alias currentIndex: bar.currentIndex
    RowLayout{
        id:browserLayout
        anchors.fill: parent
        spacing: 0
        Rectangle{
            id:leftMenuBar
            color: GTheme.bgPage
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            // 右侧分隔线
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: GTheme.borderLight
            }
            Text {
                id: title
                text: qsTr("Task")
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.top: parent.top
                anchors.topMargin: 30
                font.pixelSize: 14
                color: GTheme.textPrimary

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
            color: GTheme.bgWhite
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
