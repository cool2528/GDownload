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
        color: GTheme.bgWhite
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
            delegate: GCard {
                height: 105
                padding: 8
                outlined: true
                hoverEnabled: true
                selected: ListView.isCurrentItem || (downloadView.pageType === 0 && model.taskState === 1)
                anchors {
                    left: parent.left
                    leftMargin: 20
                    right: parent.right
                    rightMargin: 20
                }

                Column {
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
                            color: GTheme.textPrimary
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
                            // 暗色主题下 hover 不使用极浅的主色浅层，避免过曝；统一用填充浅色
                            color: mouse.hovered ? (GTheme.dark ? GTheme.fillLight : GTheme.fillLight)
                                               : (GTheme.dark ? GTheme.fillBase : GTheme.bgWhite)
                            border.color: GTheme.borderLight
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

                                    iconColor: mouse.hovered ? GTheme.textPrimary : GTheme.textSecondary
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

                                    iconColor: mouse.hovered ? GTheme.textPrimary : GTheme.textSecondary
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
                                    iconColor: mouse.hovered ? GTheme.textPrimary : GTheme.textSecondary
                                    onClicked: {
                                        if(downloadView.pageType == 0){
                                            BrowserManager.RemoveTask(0,model.taskId)
                                        }else if(downloadView.pageType == 1){
                                            BrowserManager.RemoveTask(1,model.taskId)
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
                                    iconColor: mouse.hovered ? GTheme.textPrimary : GTheme.textSecondary
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
                                    iconColor: mouse.hovered ? GTheme.textPrimary : GTheme.textSecondary
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
                            bkColor: GTheme.fillLight
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
                            color: GTheme.textSecondary
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
                                color: GTheme.textSecondary
                                font.pixelSize: 14
                            }

                            Text {
                                id: downloadRemaining
                                Layout.topMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: qsTr("Remaining ") + model.remainingTime //qsTr("Remaining 1h 37m 21s")
                                color: GTheme.textSecondary
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
                                color: GTheme.textSecondary
                            }

                            Text {
                                id: connectNumber
                                Layout.leftMargin: 10
                                Layout.topMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: String("%1").arg(model.connections) //qsTr("64")
                                color: GTheme.textSecondary
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
