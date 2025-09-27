import QtQuick
import gdl.sdk
import QtQuick.Layouts
import QtQuick.Controls
import "../Utils/utils.js" as Utils
Popup {
    id:taskPage
    width: 640
    height: flick.contentHeight
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    padding: 0
    closePolicy:Popup.CloseOnEscape //| Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    focus: true
    background: Rectangle{
        color: GTheme.bgWhite
    }
    contentItem: ScrollView {
        id:scrollView
        clip: true
        width: parent.width
        height: parent.height
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentItem: Flickable{
            id:flick
            implicitWidth: flick.width
            contentHeight: columnLayout.height
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout{
                id:columnLayout
                width: parent.width
                ListView{
                    id:tabTitle
                    orientation: ListView.Horizontal
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 20
                    Layout.topMargin: 10
                    spacing: 20
                    Layout.maximumHeight: 40
                    Layout.preferredHeight: 40
                    clip:true
                    focus: true
                    interactive: false
                    model: [qsTr("URL"),qsTr("Torent"),qsTr("Baidu")]
                    displaced: Transition { NumberAnimation { properties: "x"; duration: 300; easing: Easing.OutExpo } }
                    delegate: Item {
                        id: delegateItem
                        width: textMetrics.width
                        height: tabTitle.height
                        Button{
                            anchors.fill: parent
                            contentItem: Text {
                                id: name
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData
                                color: (index === tabTitle.currentIndex || parent.hovered) ? GTheme.primaryColor : GTheme.textRegular
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }
                            background: MouseArea{
                                cursorShape: Qt.PointingHandCursor;
                                acceptedButtons: Qt.NoButton
                            }
                            onClicked: {
                                tabTitle.currentIndex = index
                            }
                        }
                        TextMetrics{
                            id:textMetrics
                            font.pixelSize: name.font.pixelSize
                            text: modelData
                        }
                    }
                    highlight: Rectangle{
                        height: 2
                        y: tabTitle.currentItem ? tabTitle.currentItem.y + tabTitle.currentItem.height - 2 : 0
                        x: tabTitle.currentItem ? tabTitle.currentItem.x : 0
                        z:999
                        width: tabTitle.currentItem ? tabTitle.currentItem.width : 0
                        color: GTheme.primaryColor
                        Behavior on x {
                            NumberAnimation {
                                easing.type: Easing.OutExpo
                                duration: 300
                            }
                        }
                        Behavior on width {
                            NumberAnimation {
                                easing.type: Easing.OutExpo
                                duration: 300
                            }
                        }
                    }
                    highlightFollowsCurrentItem: false

                }
                Rectangle{
                    id:splitLine
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    height: 2
                    z:tabTitle.z - 1
                    color:  GTheme.borderLight
                }
                // tab Page
                StackLayout{
                    id:taskPageLayout
                    currentIndex: tabTitle.currentIndex
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    height: 300
                    property int urlType: 0
                    Rectangle{
                        id:linkingPage
                        color: GTheme.bgWhite
                        TextArea{
                            id:input
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            font.pixelSize: 12
                            placeholderText: qsTr("One task url per line (supports magnet)")
                            color: GTheme.textPrimary
                            placeholderTextColor: GTheme.textPlaceholder
                            background: Rectangle{
                                implicitHeight: taskPageLayout.height
                                implicitWidth: taskPageLayout.width
                                color: GTheme.bgWhite
                                border.color: input.enabled ? GTheme.primaryColor : GTheme.borderBase
                            }
                        }
                        Component.onCompleted: {
                            // 监听剪贴板
                            ClipboardWatcher.clipboardChanged.connect(function(text){
                                 if(text.length > 3){
                                     input.text = text
                                 }
                            })
                            input.text = ClipboardWatcher.GetClipboardText()
                        }
                    }
                    Rectangle{
                        id:seedPage
                        GDropArea{
                            anchors.fill: parent
                            visible: true
                            id:dropTorent
                            onAccepted: {
                                let model =  BrowserManager.GetFilePreviewModel(dropTorent.path)
                                if(model){
                                    dropTorent.visible = false
                                    filePreview.previewModel = model
                                }
                            }
                        }
                        FilePreviewList {
                            id: filePreview
                            anchors.fill: parent
                            visible: !dropTorent.visible
                            onClearRequested: {
                                filePreview.previewModel.clear()
                                filePreview.previewModel = null
                                dropTorent.visible = true
                            }
                        }
                        color: GTheme.bgWhite
                    }
                    Rectangle{
                        id:netDiskPage
                        color: GTheme.bgWhite
                        NetDiskPageView{
                            id:netDiskPageView
                             anchors.fill: parent
                        }

                    }

                    function geturls(){
                        if(currentIndex === 0){
                            taskPageLayout.urlType = 0
                            return Utils.splitPath(input.text)
                        }else{
                            taskPageLayout.urlType = 1
                            let ext = dropTorent.path.split('.').pop()
                            if(ext === "metalink" || ext === "meta4"){
                                taskPageLayout.urlType = 1
                            }else{
                                taskPageLayout.urlType = 2
                            }
                            return dropTorent.path
                        }
                    }

                    function getOptions(){
                        let options = {}
                        let headers = []
                        if(renameEdit.text.length > 0){
                            options["out"] = renameEdit.text
                        }
                        if(spinbox.value > 0){
                            options["max-concurrent-downloads"] = String("%1").arg(spinbox.value)
                        }
                        if(savePath.path.length > 0){
                            options["dir"] = savePath.path
                        }
                        if(userAgent.text.length > 0){
                            options["user-agent"] = userAgent.text
                        }
                        if(authorization.text.length > 0){
                            options["http-auth-challenge"] = "true"
                            headers.push(String("Authorization: %1").arg(authorization.text))
                        }
                        if(cookie.text.length > 0){
                            headers.push(String("Cookie: %1").arg(cookie.text))
                        }
                        if(referrer.text.length > 0){
                            options["referer"] = referrer.text
                        }
                        if(currentIndex === 1){
                            let select_files = filePreview.previewModel.getSelectedFiles()
                            options["select-file"] = select_files.join()
                        }
                        // 添加自定义请求头
                        let customHeaders = customRequestHeaderList.getRequestHeaderList()
                        if (customHeaders.length > 0) {
                            headers = headers.concat(customHeaders)
                        }
                        // 设置所有headers
                        if (headers.length > 0) {
                            options["header"] = headers
                        }
                        return options
                    }
                }

                //General Configuration
                ColumnLayout{
                    id:generalConfig
                    visible: taskPageLayout.currentIndex != 2
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    GridLayout{
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 30
                        rowSpacing: 10
                        // rename
                        Label{
                            id:rename
                            Layout.leftMargin: 10
                            text: qsTr("Rename:")
                            font.pixelSize: 14
                            color: GTheme.textRegular
                        }
                        RowLayout{
                            id:renameLayout
                            GTextField{
                                id:renameEdit
                                placeholderText: qsTr("Optional")
                                Layout.fillWidth: true
                                Layout.maximumHeight: 30
                                Layout.preferredHeight: 30
                            }
                            Label{
                                id:splits
                                text: qsTr("Splits:")
                                font.pixelSize: 14
                                color: GTheme.textRegular
                            }
                            GSpinBox{
                                id:spinbox
                                from: 1
                                to:64
                                value: 64
                                focus: true
                            }
                        }
                        Label{
                            id:save
                            Layout.leftMargin: 10
                            text: qsTr("Save to:")
                            font.pixelSize: 14
                            color: GTheme.textRegular
                        }
                        FolderSelector{
                            id:savePath
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            path:SettingsManager.qDir
                        }
                    }
                }


                //Additional Configuration
                ColumnLayout {
                    id: additionalConfig
                    visible: advanced.checked && taskPageLayout.currentIndex != 2
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10

                        Label {
                            text: "User-Agent:"
                            font.pixelSize: 14
                            color: GTheme.textRegular
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            id: userAgent
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "User-Agent"
                        }

                        Label {
                            text: "Authorization:"
                            font.pixelSize: 14
                            color: GTheme.textRegular
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            id: authorization
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "Authorization"
                        }

                        Label {
                            text: "Referer:"
                            font.pixelSize: 14
                            color: GTheme.textRegular
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            id: referrer
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "Referer"
                        }

                        Label {
                            text: "Cookie:"
                            font.pixelSize: 14
                            color: GTheme.textRegular
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            id: cookie
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: "Cookie"
                        }

                        Label{
                            text: qsTr("Custom Request Header List:")
                            font.pixelSize: 14
                            color: GTheme.textRegular
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        TextArea{
                            id:customRequestHeaderList
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            Layout.preferredHeight: 100
                            placeholderText: qsTr("Custom request header list (one per line in the format KEY:VALUE)")
                            color: GTheme.textPrimary
                            placeholderTextColor: GTheme.textPlaceholder
                            background: Rectangle{
                                implicitHeight: taskPageLayout.height
                                implicitWidth: taskPageLayout.width
                                color: GTheme.bgWhite
                                border.color: input.enabled ? GTheme.primaryColor : GTheme.borderBase
                            }

                            function getRequestHeaderList() {
                                let headers = []
                                if (customRequestHeaderList.text.trim().length === 0) {
                                    return headers
                                }
                                let lines = customRequestHeaderList.text.split('\n')
                                for (let line of lines) {
                                    line = line.trim()
                                    if (line.length === 0) continue
                                    let colonIndex = line.indexOf(':')
                                    if (colonIndex === -1) continue
                                    let key = line.substring(0, colonIndex).trim()
                                    let value = line.substring(colonIndex + 1).trim()
                                    if (key.length === 0 || value.length === 0) continue
                                    headers.push(key + ": " + value)
                                }
                                return headers
                            }
                        }
                    }
                }

                // submit layout
                RowLayout{
                    id:submitLayout
                    spacing: 10
                    Layout.bottomMargin: 20
                    GCheckBox{
                        id:advanced
                        visible: taskPageLayout.currentIndex != 2
                        Layout.maximumWidth: 150
                        Layout.alignment: Qt.AlignLeft
                        Layout.leftMargin: 10
                        Layout.preferredHeight: 15
                        text: qsTr("Advanced Options")
                    }
                    Item {
                        id: placeholder
                        Layout.fillWidth: true
                    }
                    GButton{
                        id:cancel
                        type: 0
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: 70
                        text: qsTr("Cancel")
                        onClicked:taskPage.close()
                    }

                    GButton{
                        id:submit
                        type: 1
                        Layout.alignment: Qt.AlignRight
                        Layout.rightMargin: 10
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: 70
                        text: qsTr("Submit")
                        onClicked: {
                            if(taskPageLayout.currentIndex !== 2){
                                let url = taskPageLayout.geturls()
                                let options = taskPageLayout.getOptions()
                                if(taskPageLayout.urlType === 0)
                                {
                                    BrowserManager.AddHttpTask(url,options)
                                }else if(taskPageLayout.urlType === 1){
                                    BrowserManager.AddMetalinkTask(url,options)
                                }else if(taskPageLayout.urlType === 2){
                                    BrowserManager.AddTorrentTask(url,options)
                                }
                            }else{
                                NetWorkDiskManager.DownloadSelectedFiles()
                            }
                            brower_view.index = 0
                            brower_view.switchDownloadPage(0)
                            if(taskPageLayout.currentIndex !== 2){
                                taskPage.close()
                            }

                        }
                    }

                }
            }
        }
    }
}
