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

    // 页面级布局常量:网盘解析页的行高、列宽与图标尺寸只服务本页面
    readonly property int parseRowHeight: GTheme.sizeLarge
    readonly property int rowHeight: GTheme.sizeDefault
    readonly property int fileIconSize: GTheme.fontTitle
    readonly property int fileNameMinWidth: 200
    readonly property int fileSizeMinWidth: 100
    readonly property int fileDateMinWidth: 100

    Rectangle{
        id:topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        color: GTheme.bgWhite
        height: visible ? netDiskPage.parseRowHeight : 0
        visible: true
        RowLayout {
            id:parseLayout
            Layout.topMargin: 0
            spacing: GTheme.spaceSM
            TextArea{
                id:urlInput
                Layout.preferredWidth: netDiskPage.width - parseUrlBtn.width - GTheme.spaceSM
                Layout.fillHeight: true
                font.pixelSize: GTheme.fontCaption
                placeholderText: qsTr("Baidu Netdisk share link format (https://pan.baidu.com/s/1xxxxxxxxxx/?pwd=xxxx)")
                color: GTheme.textPrimary
                placeholderTextColor: GTheme.textPlaceholder
                background: Rectangle{
                    implicitHeight: parent.height
                    implicitWidth: parent.width
                    color: GTheme.fillLighter
                    radius: GTheme.radiusBase
                    border.width: 1
                    border.color: urlInput.activeFocus ? GTheme.primaryColor : GTheme.borderLight

                    Behavior on border.color {
                        ColorAnimation { duration: GTheme.durationBase }
                    }
                }
            }

            // 解析按钮
            GButton {
                id: parseUrlBtn
                type: 1
                Layout.preferredHeight: GTheme.sizeDefault
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
        implicitHeight: tipLayout.implicitHeight
        height: visible ? implicitHeight : 0
        visible: topBar.visible
        color: "transparent"
        ColumnLayout{
            id: tipLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceSM
            spacing: GTheme.spaceSM
            Label{
                id:tipLabel
                text: qsTr("Precautions for parsing Baidu Netdisk share links:")
                color: GTheme.textDanger
                font.pixelSize: GTheme.fontTitle
                font.weight: GTheme.weightDemiBold
            }
            Label{
                text: qsTr("1.Please go to Software Settings -> Advanced Settings -> Set Baidu Netdisk cookies")
                color: GTheme.textDanger
                font.pixelSize: GTheme.fontBody
            }
            Label{
                text: qsTr("2.Baidu Netdisk share link format (https://pan.baidu.com/s/1xxxxxxxxxx/?pwd=xxxx)")
                color: GTheme.textDanger
                font.pixelSize: GTheme.fontBody
            }
            Label{
                text: qsTr("3.Please ensure that your account has sufficient storage space before downloading, as the file needs to be saved to your cloud drive first.")
                color: GTheme.textDanger
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width - GTheme.space3XL
                font.pixelSize: GTheme.fontBody
                wrapMode: Text.Wrap
                elide: Text.ElideNone
            }
            Label{
                text: qsTr("4.Unable to achieve accelerated downloading, only standard downloading is supported. For high-speed downloads, please purchase the official VIP.")
                color: GTheme.textDanger
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width - GTheme.space3XL
                font.pixelSize: GTheme.fontBody
                wrapMode: Text.Wrap
                elide: Text.ElideNone
            }

            GButton{
                type: 1
                text: qsTr("Click me to go to settings")
                Layout.alignment: Qt.AlignCenter
                Layout.preferredHeight: GTheme.sizeDefault
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
            if(taskType === 0){
                //ParseShareUrl
                if(isSuccess){
                    Qt.callLater(function(){
                        topBar.visible = false
                        fileListView.forceLayout()
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
                        // 点击会自动 toggle checked 破坏绑定，之后 selectAll/unselectAll 不再同步，需重建
                        checked = Qt.binding(function() { return model.isSelected })
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
                    // 旧逻辑用 parentPath.length >= homePath.length 判断层级，字符串长度不等于路径深度，
                    // 边界情况（如 /a/bc 与 /a/b/c 长度相近）会误判。改为：
                    // 当 getParentPath 返回值与传入值相同即已到根，不再回退。
                    if(netDiskPage.parentPath.length > 0){
                        let next = Utils.getParentPath(netDiskPage.parentPath)
                        if(next.length > 0 && next !== netDiskPage.parentPath){
                            netDiskPage.parentPath = next
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
