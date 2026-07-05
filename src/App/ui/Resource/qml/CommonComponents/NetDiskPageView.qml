import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
import "../Utils/utils.js" as Utils
Rectangle {
    id: netDiskPage
    objectName: "netDiskPage"
    color: GTheme.bgWhite
    property string parentPath:""
    property string homePath:""
    property bool isBusy: false

    // 解析/浏览两态的单一状态源:true=粘贴解析态(显示输入行+工作流引导卡),
    // false=文件浏览态(显示文件列表)。原实现命令式写 topBar.visible=false 切换,
    // 导致 visible 绑定被破坏且多处可见性彼此耦合脆弱;统一由本属性驱动。
    property bool parseMode: true

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
        visible: netDiskPage.parseMode
        RowLayout {
            id:parseLayout
            anchors.fill: parent
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
                    radius: GTheme.radiusMedium
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

    // 解析工作流引导卡:三步流程 + Cookie 告警 + 去设置入口
    // 关键:卡片被 anchors 定位(非置于父 Layout 中),不能用 anchors.fill 的内层
    // Layout 反向驱动卡片高度 —— 那会与卡片高度形成绑定循环,Qt 断环后 implicitHeight
    // 归零导致整卡坍缩。此处内层 ColumnLayout 仅锚 top/left/right(不 fill、无 bottom),
    // 其高度即自身 implicitHeight(只由内容决定,不读卡片高度);卡片高度再据此显式计算,
    // 彻底断开循环。
    GCard {
        id: workflowCard
        objectName: "netDiskWorkflow"
        // 锚到页面顶部 + 固定偏移(输入行高 + 间距),不依赖 topBar.height 的渲染时序
        anchors.top: parent.top
        anchors.topMargin: netDiskPage.parseRowHeight + GTheme.spaceLG
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: GTheme.spaceLG
        anchors.rightMargin: GTheme.spaceLG
        visible: netDiskPage.parseMode
        height: workflowColumn.implicitHeight + GTheme.spaceLG * 2
        variant: "elevated"
        outlined: true
        padding: GTheme.spaceLG
        radius: GTheme.radiusRound
        hoverEnabled: false
        shadow: true

        ColumnLayout {
            id: workflowColumn
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: GTheme.spaceMD

            Text {
                text: qsTr("Cloud link parser")
                font.pixelSize: GTheme.fontTitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                Layout.fillWidth: true
            }

            Text {
                text: qsTr("Paste a Baidu share link, preview files, then add selected items to the queue.")
                font.pixelSize: GTheme.fontBody
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceMD

                Repeater {
                    model: [
                        { indexText: "1", title: qsTr("Paste link"), description: qsTr("Validate share URL and cookie."), accent: "primary" },
                        { indexText: "2", title: qsTr("Preview files"), description: qsTr("Select files before downloading."), accent: "info" },
                        { indexText: "3", title: qsTr("Add queue"), description: qsTr("Send selected files to aria2."), accent: "success" }
                    ]

                    GCard {
                        required property var modelData
                        Layout.fillWidth: true
                        // 显式 implicitHeight:内容用 anchors.fill 不会反向撑高卡片,
                        // 必须给定高度否则卡片坍缩(参照 QuickActionCard)
                        implicitHeight: GTheme.sizeLarge + GTheme.spaceMD * 3
                        padding: GTheme.spaceMD
                        outlined: true
                        hoverEnabled: false
                        variant: modelData.accent === "success" ? "accentSuccess" : (modelData.accent === "info" ? "accentInfo" : "accentPrimary")

                        RowLayout {
                            anchors.fill: parent
                            spacing: GTheme.spaceSM

                            Rectangle {
                                Layout.preferredWidth: GTheme.sizeDefault
                                Layout.preferredHeight: GTheme.sizeDefault
                                Layout.alignment: Qt.AlignVCenter
                                radius: GTheme.radiusMedium
                                color: GTheme.bgWhite
                                border.width: 1
                                border.color: GTheme.borderLight

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.indexText
                                    font.pixelSize: GTheme.fontBody
                                    font.weight: GTheme.weightDemiBold
                                    color: GTheme.primaryColor
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: GTheme.spaceXS

                                Text {
                                    text: modelData.title
                                    font.pixelSize: GTheme.fontBody
                                    font.weight: GTheme.weightMedium
                                    color: GTheme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: modelData.description
                                    font.pixelSize: GTheme.fontCaption
                                    color: GTheme.textSecondary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            AlertTip {
                Layout.fillWidth: true
                severity: SettingsManager.qBaiduPanCookies.length === 0 ? "warning" : "info"
                text: SettingsManager.qBaiduPanCookies.length === 0
                      ? qsTr("Cookie required. Set Baidu Netdisk cookies in Preferences before parsing share links.")
                      : qsTr("Ready to parse. The parser will preview files before adding them to the download queue.")
            }

            GButton {
                text: qsTr("Open Baidu cookie settings")
                visible: SettingsManager.qBaiduPanCookies.length === 0
                buttonType: "primary"
                Layout.alignment: Qt.AlignRight
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
                        netDiskPage.parseMode = false
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
        height: netDiskPage.rowHeight
        RowLayout{
            id:headerLayout
            anchors.fill: parent
            spacing: GTheme.spaceSM
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
                Layout.minimumWidth: netDiskPage.fileNameMinWidth
                color: GTheme.textRegular
                font.pixelSize: GTheme.fontBody
            }
            Label {
                text: qsTr("Size")
                Layout.fillWidth: true
                Layout.minimumWidth: netDiskPage.fileSizeMinWidth
                color: GTheme.textRegular
                font.pixelSize: GTheme.fontBody
            }
            Label {
                text: qsTr("Date")
                Layout.fillWidth: true
                Layout.minimumWidth: netDiskPage.fileDateMinWidth
                color: GTheme.textRegular
                font.pixelSize: GTheme.fontBody
            }
        }

    }
    ListView{
        id:fileListView
        objectName: "netDiskFileList"
        visible: !netDiskPage.parseMode
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        clip: true
        model: NetWorkDiskManager.GetNetWorkDiskModel()
        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
        delegate: Rectangle{
            width: fileListView.width
            height: netDiskPage.rowHeight
            RowLayout{
                anchors.fill: parent
                spacing: GTheme.spaceXS
                GCheckBox {
                    checked: model.isSelected
                    onClicked: {
                        NetWorkDiskManager.ToggleSelection(index,!model.isSelected)
                        // 点击会自动 toggle checked 破坏绑定，之后 selectAll/unselectAll 不再同步，需重建
                        checked = Qt.binding(function() { return model.isSelected })
                    }
                }

                Image {
                    width: netDiskPage.fileIconSize
                    height: netDiskPage.fileIconSize
                    source: model.isDir ? "/images/browser/FolderType.png" : "/images/browser/OtherType.png"
                }

                Label {
                    text: model.fileName
                    Layout.fillWidth: true
                    Layout.minimumWidth: netDiskPage.fileNameMinWidth
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                }

                Label {
                    text: model.fileSize
                    Layout.fillWidth: true
                    Layout.minimumWidth: netDiskPage.fileSizeMinWidth
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                }

                Label {
                    text: model.createTime
                    Layout.fillWidth: true
                    Layout.minimumWidth: netDiskPage.fileDateMinWidth
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                }

            }
            MouseArea{
                anchors.left: parent.left
                anchors.leftMargin: netDiskPage.rowHeight
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
        anchors.bottom: parent.bottom
        width: fileListView.width
        visible: fileListView.visible
        height: netDiskPage.rowHeight
        RowLayout{
            anchors.fill: parent
            spacing: GTheme.spaceSM
            GButton{
                text: qsTr("Back")
                Layout.preferredHeight: GTheme.sizeDefault
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
                Layout.preferredHeight: GTheme.sizeDefault
                onClicked: {
                    netDiskPage.parseMode = true
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
