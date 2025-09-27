import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
import "../Utils/utils.js" as Utils
Rectangle {
    id: netDiskPage
    color: GTheme.bgWhite
    property string parentPath:""
    property string homePath:""
    property bool isBusy: false

    Rectangle{
        id:topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        color: GTheme.bgWhite
        height: visible ? 50 : 0
        visible: true
        RowLayout {
            id:parseLayout
            Layout.topMargin: 0
            spacing: 10
            TextArea{
                id:urlInput
                Layout.preferredWidth: netDiskPage.width - parseUrlBtn.width - 10
                Layout.fillHeight: true
                font.pixelSize: 12
                placeholderText: qsTr("Baidu Netdisk share link format (https://pan.baidu.com/s/1xxxxxxxxxx/?pwd=xxxx)")
                color: GTheme.textPrimary
                placeholderTextColor: GTheme.textPlaceholder
                background: Rectangle{
                    implicitHeight: parent.height
                    implicitWidth: parent.width
                    color: GTheme.bgWhite
                    border.color: urlInput.enabled ? GTheme.primaryColor : GTheme.borderBase
                }
            }

            // 解析按钮
            GButton {
                id: parseUrlBtn
                type: 1
                text: qsTr("Parse")
                onClicked: {
                    if(!checkShareUrl(urlInput.text)){
                        ToastManager.ShowError(qsTr("Invalid Baidu Netdisk URL, please check."))
                        return
                    }
                    if(SettingsManager.qBaiduPanCookies.length === 0){
                        ToastManager.ShowError(qsTr("Please set Baidu Netdisk cookies first."))
                        return
                    }
                    netDiskPage.homePath = ""
                    netDiskPage.parentPath = ""
                    let shareUrl = Utils.removeNewlineAndTrim(urlInput.text)
                    NetWorkDiskManager.ParseShareUrl(shareUrl)
                    netDiskPage.isBusy = true
                }
            }
        }
    }

    Rectangle{
        id:tipRect
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 100
        visible: topBar.visible
        color: "transparent"
        ColumnLayout{
            anchors.fill: parent
            spacing: 10
            Label{
                id:tipLabel
                text: qsTr("Precautions for parsing Baidu Netdisk share links:")
                color: GTheme.dangerColor
                font.pixelSize: 18
                font.bold: true
            }
            Label{
                text: qsTr("1.Please go to Software Settings -> Advanced Settings -> Set Baidu Netdisk cookies")
                color: GTheme.dangerColor
                font.pixelSize: 15
            }
            Label{
                text: qsTr("2.Baidu Netdisk share link format (https://pan.baidu.com/s/1xxxxxxxxxx/?pwd=xxxx)")
                color: GTheme.dangerColor
                font.pixelSize: 15
            }
            Label{
                text: qsTr("3.Please ensure that your account has sufficient storage space before downloading, as the file needs to be saved to your cloud drive first.")
                color: GTheme.dangerColor
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width - 50
                font.pixelSize: 15
                wrapMode: Text.Wrap
                elide: Text.ElideNone
            }
            Label{
                text: qsTr("4.Unable to achieve accelerated downloading, only standard downloading is supported. For high-speed downloads, please purchase the official VIP.")
                color: GTheme.dangerColor
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width - 50
                font.pixelSize: 15
                wrapMode: Text.Wrap
                elide: Text.ElideNone
            }

            GButton{
                type: 1
                text: qsTr("Click me to go to settings")
                Layout.alignment: Qt.AlignCenter
                onClicked: {
                    brower_view.index = 1
                    brower_view.switchSettingPage(1)
                }
            }
        }
    }

    Connections{
        id:netDiskConnections
        target: NetWorkDiskManager
        function onTaskFinished(msg,isSuccess,taskType){
            netDiskPage.isBusy = false
            console.log("busyIndicator ",busyIndicator.height ," ",busyIndicator.x," ",busyIndicator.y)
            if(taskType === 0){
                //ParseShareUrl
                if(isSuccess){
                    Qt.callLater(function(){
                        topBar.visible = false
                        fileListView.forceLayout()
                        console.log("busyIndicator ",busyIndicator.height ," ",busyIndicator.x," ",busyIndicator.y)
                    })

                }else{
                    ToastManager.ShowError(msg)
                }
            }else if(taskType === 1){
                // EnterDirectory
                if(isSuccess){
                    Qt.callLater(function(){
                        fileListView.forceLayout()
                    })

                }else{
                    ToastManager.ShowError(msg)
                }
            }else if(taskType === 2){
                //GetDownloadInfo
                if(isSuccess){

                }else{
                    ToastManager.ShowError(msg)
                }
            }

        }
    }

    BusyIndicator{
        id:busyIndicator
        anchors.centerIn: parent
        height: running ? implicitHeight : 0
        running: netDiskPage.isBusy
        z: 1
    }
    Rectangle{
        id:header
        width: fileListView.width
        visible: fileListView.visible
        anchors.top: parent.top
        height: 30
        RowLayout{
            id:headerLayout
            anchors.fill: parent
            spacing: 10
            GCheckBox {
                id:selectAllCheckBox
                onClicked: {
                    if(checked){
                        NetWorkDiskManager.SelectAll()
                    }else{
                        NetWorkDiskManager.UnselectAll()
                    }
                }
            }
            Label {
                text: qsTr("File Name")
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                color: GTheme.textRegular
                font.pixelSize: 14
            }
            Label {
                text: qsTr("Size")
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                color: GTheme.textRegular
                font.pixelSize: 14
            }
            Label {
                text: qsTr("Date")
                Layout.fillWidth: true
                Layout.minimumWidth: 100
                color: GTheme.textRegular
                font.pixelSize: 14
            }
        }

    }
    ListView{
        id:fileListView
        visible: !topBar.visible
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: 200
        clip: true
        model: NetWorkDiskManager.GetNetWorkDiskModel()
        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
        delegate: Rectangle{
            width: fileListView.width
            height: 30
            RowLayout{
                anchors.fill: parent
                spacing: 5
                GCheckBox {
                    checked: model.isSelected
                    onClicked: {
                        NetWorkDiskManager.ToggleSelection(index,!model.isSelected)
                    }
                }

                Image {
                    width: 20
                    height: 20
                    source: model.isDir ? "/images/browser/FolderType.png" : "/images/browser/OtherType.png"
                }

                Label {
                    text: model.fileName
                    Layout.fillWidth: true
                    Layout.minimumWidth: 200
                    color: GTheme.textPrimary
                    font.pixelSize: 14
                }

                Label {
                    text: model.fileSize
                    Layout.fillWidth: true
                    Layout.minimumWidth: 100
                    color: GTheme.textPrimary
                    font.pixelSize: 14
                }

                Label {
                    text: model.createTime
                    Layout.fillWidth: true
                    Layout.minimumWidth: 100
                    color: GTheme.textPrimary
                    font.pixelSize: 14
                }

            }
            MouseArea{
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if(model.isDir){
                        netDiskPage.parentPath = model.filePath
                        NetWorkDiskManager.ChangeDir(model.filePath,model.fileId)
                        netDiskPage.isBusy = true
                    }
                }
            }
            Component.onCompleted: {
                if(netDiskPage.homePath.length === 0){
                    netDiskPage.homePath = Utils.getParentPath(model.filePath)
                }
            }
        }
    }

    Rectangle{
        id:footer
        anchors.top: fileListView.bottom
        anchors.bottom: parent.bottom
        width: fileListView.width
        visible: fileListView.visible
        height: 30
        RowLayout{
            anchors.fill: parent
            spacing: 10
            GButton{
                text: qsTr("Back")
                onClicked: {
                    if(netDiskPage.parentPath.length > 0 && netDiskPage.parentPath.length >= netDiskPage.homePath.length){
                        netDiskPage.parentPath = Utils.getParentPath(netDiskPage.parentPath)
                        if(netDiskPage.parentPath.length != 0){
                            NetWorkDiskManager.ChangeDir(netDiskPage.parentPath,"")
                            netDiskPage.isBusy = true
                        }
                    }
                }
            }
            GButton{
                text: qsTr("Return parsing")
                type: 1
                onClicked: {
                    topBar.visible = true
                    netDiskPage.parentPath = ""
                    netDiskPage.homePath = ""
                }
            }
        }
    }


    // Baidu Netdisk share link format (https://pan.baidu.com/s/1xxxxxxxxxx/?pwd=xxxx)
    function checkShareUrl(url) {
        var reg = /https:\/\/pan\.baidu\.com\/s\/[A-Za-z0-9_-]+(\?pwd=[A-Za-z0-9]+)?/;
        return reg.test(url);
    }
}
