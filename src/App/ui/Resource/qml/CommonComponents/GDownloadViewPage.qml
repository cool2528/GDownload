import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
Control{
    id:downloadView
    property alias model: listViewdownload.model
    property int pageType: -1 // 0 downloadPage 1 waitingPage  2 completedPage
    width: parent.width
    height: parent.height
    background: Rectangle{
        color: GTheme.dark ? "#2e2e2e" :"#ffffff"
        Item {
            anchors.fill: parent
            opacity: listViewdownload.count > 0 ? 0 : 1
            onOpacityChanged: {
                console.log("page_background opacity ",opacity)
            }
            Image {
                id: backgroundImage
                anchors.centerIn: parent
                source: "/images/browser/no-task.svg"
            }
        }

    }

    ScrollView{
        id:scroView
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        width: parent.width
        height: parent.height
        ListView{
            id:listViewdownload
            Layout.fillWidth: true
            height: downloadView.height
            spacing: 10
            clip:true
            focus: true
            interactive: false
            orientation:ListView.Vertical
            model:downloadView.model
            delegate:Rectangle{
                height: 105
                color: GTheme.dark ? "#282828" : "#ffffff"
                radius: 5
                anchors{
                    left: parent.left
                    leftMargin: 20
                    right: parent.right
                    rightMargin: 20

                }
                border.color: itemMouse.hovered ? "#5151f9" : GTheme.dark ? "#4b4b4b" : "#c5c5c5"
                HoverHandler{
                    id:itemMouse
                    acceptedDevices: PointerDevice.Mouse
                    cursorShape: Qt.ArrowCursor
                }

                Column{
                    anchors.fill: parent
                    spacing: 10
                    RowLayout{
                        id:rowlayout
                        width: parent.width
                        height: 30
                        spacing: 10
                        Text {
                            id: titleName
                            text: model.fileName
                            Layout.leftMargin: 20
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.margins: 10
                            color: GTheme.dark ? "#ffffff" : "#303133"
                            font.pixelSize: 14
                        }
                        Rectangle{
                            id:controlRect
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            Layout.rightMargin: 10
                            //Layout.fillWidth: true
                            implicitWidth: 150
                            radius: 5
                            Layout.preferredHeight: 25
                            color: GTheme.dark ? mouse.hovered ? "#5151f9" : "#414141" : mouse.hovered ? "#5151f9" : "#ffffff"
                            border.color: GTheme.dark ? "#545454" : "#f5f5f5"
                            HoverHandler{
                                id:mouse
                                acceptedDevices: PointerDevice.Mouse
                                cursorShape: Qt.ArrowCursor
                            }
                            // download page
                            RowLayout{
                                spacing: 5
                                Layout.alignment: Qt.AlignVCenter
                                width: parent.width
                                height: parent.height
                                IconButton{
                                    id:revocerButton
                                    visible: model.taskState !== 1
                                    Layout.leftMargin: 10
                                    Layout.margins: 5
                                    //Layout.fillWidth: true
                                    Layout.minimumHeight: 20
                                    Layout.maximumHeight: 20
                                    iconSource: SegoeFluentIcons.Play

                                    iconColor:GTheme.dark ? mouse.hovered ? "#ffffff" : "#7c7c7c": mouse.hovered ? "#ffffff" : "#acacac"
                                    onClicked: {
                                        if(downloadView.pageType == 0)
                                        {
                                            BrowserManager.UnpauseTask(0,model.taskId)
                                        }else if(downloadView.pageType == 1){
                                            //
                                            BrowserManager.UnpauseTask(1,model.taskId)
                                        }else if(downloadView.pageType == 2){
                                            //
                                            Qt.openUrlExternally(model.savePath)
                                        }
                                    }
                                }
                                IconButton{
                                    id:pauseButton
                                    visible: model.taskState === 1
                                    Layout.leftMargin: 10
                                    Layout.margins: 5
                                    //Layout.fillWidth: true
                                    Layout.minimumHeight: 20
                                    Layout.maximumHeight: 20
                                    iconSource: SegoeFluentIcons.Pause

                                    iconColor:GTheme.dark ? mouse.hovered ? "#ffffff" : "#7c7c7c": mouse.hovered ? "#ffffff" : "#acacac"
                                    onClicked: {
                                        if(downloadView.pageType == 0)
                                        {
                                            BrowserManager.PauseTask(0,model.taskId)
                                        }else if(downloadView.pageType == 1){
                                            //
                                            BrowserManager.PauseTask(1,model.taskId)
                                        }else if(downloadView.pageType == 2){
                                            //
                                        }
                                    }
                                }
                                IconButton{
                                    id:delButton
                                    Layout.margins: 5
                                    //Layout.fillWidth: true
                                    Layout.minimumHeight: 20
                                    Layout.maximumHeight: 20
                                    iconSource: SegoeFluentIcons.Delete
                                    iconColor:GTheme.dark ? mouse.hovered ? "#ffffff" : "#7c7c7c": mouse.hovered ? "#ffffff" : "#acacac"
                                    onClicked: {
                                        if(downloadView.pageType == 0){
                                            BrowserManager.RemoveTask(model.taskId)
                                        }else if(downloadView.pageType == 1){
                                            //
                                        }else if(downloadView.pageType == 2){
                                            //
                                            BrowserManager.RemoveStopTask(model.taskId)
                                        }
                                    }
                                }

                                IconButton{
                                    id:openFolderButton
                                    Layout.margins: 5
                                    //Layout.fillWidth: true
                                    Layout.minimumHeight: 20
                                    Layout.maximumHeight: 20
                                    iconSource: SegoeFluentIcons.Folder
                                    iconColor:GTheme.dark ? mouse.hovered ? "#ffffff" : "#7c7c7c": mouse.hovered ? "#ffffff" : "#acacac"
                                    onClicked: {
                                        BrowserManager.OpenFileLocation(model.savePath)
                                    }
                                }

                                IconButton{
                                    id:copyUrlButton
                                    Layout.topMargin: 8
                                    Layout.margins: 5
                                    //Layout.fillWidth: true
                                    Layout.minimumHeight: 20
                                    Layout.maximumHeight: 20
                                    iconSource: SegoeFluentIcons.Link
                                    iconColor:GTheme.dark ? mouse.hovered ? "#ffffff" : "#7c7c7c": mouse.hovered ? "#ffffff" : "#acacac"
                                    onClicked: {
                                        console.debug("copy download link ",model.downloadLink)
                                        UtilsToolsManager.SetClipboardText(model.downloadLink)
                                    }
                                }
                            }

                        }
                    }

                    // ProgressBar

                    RowLayout{
                        id:progressLayout
                        width: parent.width
                        height: 10
                        GProgressBar{
                            id:progressBar
                            Layout.margins: 10
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6
                            from: 0
                            to:100
                            value: model.progress
                            bkColor: GTheme.dark ? "#ffffff" : "#e8ebf3"
                        }
                    }

                    // tip text
                    RowLayout{
                        id:tipText
                        width: parent.width
                        height: 30
                        Text {
                            id: downloadProgress
                            Layout.margins: 10
                            Layout.alignment:  Qt.AlignLeft
                            Layout.fillWidth: true
                            text: model.currentSize + "/" + model.totalSize //qsTr("7.38MB/5.43GB")
                            color: GTheme.dark ? "#878787" : "#a0a0a0"
                            font.pixelSize: 14
                        }

                        RowLayout{
                            spacing: 0
                            Layout.alignment: Qt.AlignRight
                            Layout.fillWidth: true
                            Layout.maximumWidth: 300
                            Layout.preferredWidth: 200
                            Text {
                                id: downloadSpeed
                                Layout.topMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: model.downloadSpeed //qsTr("↓973.4 KB/s")
                                color: GTheme.dark ? "#878787" : "#a0a0a0"
                                font.pixelSize: 14
                            }

                            Text {
                                id: downloadRemaining
                                Layout.topMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: qsTr("Remaining ") + model.remainingTime //qsTr("Remaining 1h 37m 21s")
                                color: GTheme.dark ? "#878787" : "#a0a0a0"
                                font.pixelSize: 14
                            }

                            FontIcon{
                                id:connectedIcon
                                Layout.topMargin: 13
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                iconSource: SegoeFluentIcons.Connected
                                iconSize: 14
                                color: GTheme.dark ? "#878787" : "#a0a0a0"
                            }

                            Text {
                                id: connectNumber
                                Layout.leftMargin: 10
                                Layout.topMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: String("%1").arg(model.connections) //qsTr("64")
                                color: GTheme.dark ? "#878787" : "#a0a0a0"
                                font.pixelSize: 14
                            }
                        }
                    }
                    // end

                }

            }

        }


    }
    Component.onCompleted: {
        console.debug("model ",listViewdownload.count)
    }
}
