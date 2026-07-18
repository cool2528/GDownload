import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
import "../Utils/utils.js" as Utils

Rectangle {
    id: netDiskPage
    objectName: "netDiskPage"
    color: GTheme.bgPage

    property string parentPath: ""
    property string homePath: ""
    property bool isBusy: false
    property bool parseMode: true
    property string lastErrorText: ""
    readonly property int operationTimeoutMs: 70000

    readonly property bool compact: width < 620
    readonly property bool veryCompact: width < 460
    readonly property int rowHeight: compact ? 52 : 48
    readonly property int fileIconSize: GTheme.fontTitle
    readonly property var diskModel: NetWorkDiskManager.GetNetWorkDiskModel()
    readonly property bool modelEmpty: !diskModel || diskModel.count === 0

    function beginOperation() {
        lastErrorText = ""
        isBusy = true
        operationTimeout.restart()
    }

    Timer {
        id: operationTimeout
        interval: netDiskPage.operationTimeoutMs
        repeat: false
        onTriggered: {
            if (!netDiskPage.isBusy)
                return
            netDiskPage.isBusy = false
            netDiskPage.lastErrorText = qsTr("The cloud request timed out. Please check your network and try again.")
            ToastManager.ShowError(netDiskPage.lastErrorText)
        }
    }

    function parseShareLink() {
        if (!checkShareUrl(urlInput.text)) {
            lastErrorText = qsTr("Enter a valid Baidu Netdisk share link before parsing.")
            ToastManager.ShowError(qsTr("Invalid Baidu Netdisk URL, please check."))
            return
        }
        if (SettingsManager.qBaiduPanCookies.length === 0) {
            lastErrorText = qsTr("Baidu Netdisk cookies are required. Add them in Preferences and try again.")
            ToastManager.ShowError(qsTr("Please set Baidu Netdisk cookies first."))
            return
        }
        homePath = ""
        parentPath = ""
        NetWorkDiskManager.ParseShareUrl(Utils.removeNewlineAndTrim(urlInput.text))
        beginOperation()
    }

    Flickable {
        id: parseViewport
        objectName: "netDiskParseViewport"
        anchors.fill: parent
        visible: netDiskPage.parseMode
        clip: true
        contentWidth: width
        contentHeight: parseColumn.implicitHeight + GTheme.spaceLG * 2
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: parseColumn
            x: GTheme.spaceLG
            y: GTheme.spaceLG
            width: Math.max(0, parseViewport.width - GTheme.spaceLG * 2)
            spacing: GTheme.spaceMD

            RowLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    radius: GTheme.radiusLarge
                    color: GTheme.bgInfo
                    border.width: 1
                    border.color: GTheme.borderInfo

                    AuroraIcon {
                        anchors.centerIn: parent
                        name: "cloud"
                        iconSize: GTheme.fontTitle
                        color: GTheme.primaryColor
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceXS

                    Text {
                        text: qsTr("Cloud link parser")
                        font.pixelSize: GTheme.fontTitle
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Text {
                        text: qsTr("Preview a Baidu share safely before adding selected files to the queue.")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
            }

            GCard {
                id: parserCard
                objectName: "netDiskParserCard"
                Layout.fillWidth: true
                implicitHeight: parserContent.implicitHeight + GTheme.spaceLG * 2
                padding: GTheme.spaceLG
                outlined: true
                hoverEnabled: false
                variant: "elevated"

                ColumnLayout {
                    id: parserContent
                    anchors.fill: parent
                    anchors.margins: parserCard.padding
                    spacing: GTheme.spaceSM

                    Text {
                        text: qsTr("Baidu share link")
                        color: GTheme.textPrimary
                        font.pixelSize: GTheme.fontBody
                        font.weight: GTheme.weightMedium
                        Layout.fillWidth: true
                    }

                    TextArea {
                        id: urlInput
                        objectName: "netDiskUrlInput"
                        Layout.fillWidth: true
                        Layout.preferredHeight: netDiskPage.compact ? 72 : 48
                        wrapMode: TextEdit.WrapAnywhere
                        selectByMouse: true
                        font.pixelSize: GTheme.fontBody
                        placeholderText: qsTr("Paste https://pan.baidu.com/s/... here")
                        Accessible.name: qsTr("Baidu share link")
                        color: GTheme.textPrimary
                        placeholderTextColor: GTheme.textPlaceholder
                        background: Rectangle {
                            color: GTheme.fillLighter
                            radius: GTheme.radiusMedium
                            border.width: 1
                            border.color: netDiskPage.lastErrorText.length > 0 ? GTheme.dangerColor
                                                                                 : (urlInput.activeFocus ? GTheme.primaryColor : GTheme.borderLight)
                        }
                        onTextChanged: if (netDiskPage.lastErrorText.length > 0) netDiskPage.lastErrorText = ""
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceSM

                        Text {
                            text: qsTr("Share links may include an extraction code in the URL.")
                            color: GTheme.textSecondary
                            font.pixelSize: GTheme.fontCaption
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            visible: !netDiskPage.veryCompact
                        }

                        GButton {
                            id: parseUrlBtn
                            objectName: "netDiskParseButton"
                            text: netDiskPage.isBusy ? qsTr("Parsing...") : qsTr("Parse link")
                            type: 1
                            enabled: !netDiskPage.isBusy
                            Layout.preferredHeight: GTheme.sizeDefault
                            Layout.fillWidth: netDiskPage.veryCompact
                            onClicked: netDiskPage.parseShareLink()
                        }
                    }
                }
            }

            AlertTip {
                visible: netDiskPage.lastErrorText.length > 0
                Layout.fillWidth: true
                severity: "danger"
                text: netDiskPage.lastErrorText
            }

            GridLayout {
                id: workflowGrid
                objectName: "netDiskWorkflow"
                Layout.fillWidth: true
                columns: netDiskPage.compact ? 1 : 3
                columnSpacing: GTheme.spaceSM
                rowSpacing: GTheme.spaceSM

                Repeater {
                    model: [
                        { icon: "link", title: qsTr("Paste link"), description: qsTr("Validate the share URL and cookie.") },
                        { icon: "view", title: qsTr("Preview files"), description: qsTr("Browse folders and select files.") },
                        { icon: "cloud-download", title: qsTr("Add to queue"), description: qsTr("Send the selection to aria2.") }
                    ]

                    GCard {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: netDiskPage.compact ? 72 : 104
                        padding: GTheme.spaceMD
                        outlined: true
                        hoverEnabled: false
                        // accentPrimary 使用 primaryLight 浅色阶，暗色主题下会形成浅底浅字。
                        // 工作流卡采用主题表面，让背景和文字同时跟随 Light/Dark 语义令牌。
                        variant: "default"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: GTheme.spaceMD
                            spacing: GTheme.spaceSM

                            AuroraIcon {
                                name: modelData.icon
                                iconSize: GTheme.fontTitle
                                color: GTheme.primaryColor
                                Layout.alignment: Qt.AlignTop
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: GTheme.spaceXS
                                Text {
                                    text: modelData.title
                                    color: GTheme.textPrimary
                                    font.pixelSize: GTheme.fontBody
                                    font.weight: GTheme.weightMedium
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
                                }
                                Text {
                                    text: modelData.description
                                    color: GTheme.textSecondary
                                    font.pixelSize: GTheme.fontCaption
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            AlertTip {
                Layout.fillWidth: true
                severity: SettingsManager.qBaiduPanCookies.length === 0 ? "warning" : "success"
                text: SettingsManager.qBaiduPanCookies.length === 0
                      ? qsTr("Cookie required. Set Baidu Netdisk cookies in Preferences before parsing share links.")
                      : qsTr("Baidu Netdisk is ready. Parsed files remain local until you add them to the queue.")
            }

            GButton {
                visible: SettingsManager.qBaiduPanCookies.length === 0
                text: qsTr("Open Baidu cookie settings")
                buttonType: "primary"
                Layout.alignment: netDiskPage.compact ? Qt.AlignLeft : Qt.AlignRight
                Layout.fillWidth: netDiskPage.veryCompact
                onClicked: {
                    brower_view.index = 1
                    brower_view.switchSettingPage(1)
                }
            }
        }
    }

    ColumnLayout {
        id: browserWorkspace
        objectName: "netDiskBrowserWorkspace"
        anchors.fill: parent
        anchors.margins: GTheme.spaceLG
        visible: !netDiskPage.parseMode
        spacing: GTheme.spaceSM

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            AuroraIcon { name: "folder"; color: GTheme.primaryColor }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    text: netDiskPage.parentPath.length === 0 ? qsTr("Shared files") : qsTr("Folder contents")
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontTitle
                    font.weight: GTheme.weightDemiBold
                    Layout.fillWidth: true
                }
                Text {
                    text: netDiskPage.parentPath.length === 0 ? qsTr("Root folder") : netDiskPage.parentPath
                    color: GTheme.textSecondary
                    font.pixelSize: GTheme.fontCaption
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }
            GButton {
                text: qsTr("New link")
                onClicked: {
                    netDiskPage.parseMode = true
                    netDiskPage.parentPath = ""
                    netDiskPage.homePath = ""
                    netDiskPage.lastErrorText = ""
                }
            }
        }

        AlertTip {
            visible: netDiskPage.lastErrorText.length > 0
            Layout.fillWidth: true
            severity: "danger"
            text: netDiskPage.lastErrorText
        }

        GCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0
            outlined: true
            hoverEnabled: false

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    id: header
                    Layout.fillWidth: true
                    Layout.preferredHeight: netDiskPage.rowHeight
                    color: GTheme.fillLighter

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: GTheme.spaceMD
                        anchors.rightMargin: GTheme.spaceMD
                        spacing: GTheme.spaceSM
                        GCheckBox {
                            id: selectAllCheckBox
                            onClicked: checked ? NetWorkDiskManager.SelectAll() : NetWorkDiskManager.UnselectAll()
                        }
                        Label {
                            text: qsTr("File Name")
                            Layout.fillWidth: true
                            color: GTheme.textRegular
                            font.pixelSize: GTheme.fontCaption
                        }
                        Label {
                            text: qsTr("Size")
                            visible: !netDiskPage.veryCompact
                            Layout.preferredWidth: netDiskPage.compact ? 74 : 110
                            color: GTheme.textRegular
                            font.pixelSize: GTheme.fontCaption
                        }
                        Label {
                            text: qsTr("Date")
                            visible: !netDiskPage.compact
                            Layout.preferredWidth: 150
                            color: GTheme.textRegular
                            font.pixelSize: GTheme.fontCaption
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: fileListView
                        objectName: "netDiskFileList"
                        anchors.fill: parent
                        clip: true
                        model: netDiskPage.diskModel
                        visible: !netDiskPage.modelEmpty
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        delegate: Rectangle {
                            id: fileRow
                            objectName: "netDiskFileRow"
                            width: fileListView.width
                            height: netDiskPage.rowHeight
                            activeFocusOnTab: model.isDir
                            Accessible.role: model.isDir ? Accessible.Button : Accessible.ListItem
                            Accessible.name: model.fileName
                            Accessible.description: model.isDir
                                                    ? qsTr("Open folder")
                                                    : qsTr("Cloud file")
                            color: model.isSelected ? GTheme.bgInfo : (rowMouse.containsMouse ? GTheme.fillLighter : "transparent")
                            border.width: activeFocus ? 2 : 0
                            border.color: GTheme.focusRing

                            function openDirectory() {
                                if (!model.isDir)
                                    return
                                netDiskPage.parentPath = model.filePath
                                NetWorkDiskManager.ChangeDir(model.filePath, model.fileId)
                                netDiskPage.beginOperation()
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: GTheme.spaceMD
                                anchors.rightMargin: GTheme.spaceMD
                                spacing: GTheme.spaceSM
                                GCheckBox {
                                    checked: model.isSelected
                                    Accessible.name: qsTr("Select %1").arg(model.fileName)
                                    onClicked: {
                                        NetWorkDiskManager.ToggleSelection(index, !model.isSelected)
                                        checked = Qt.binding(function() { return model.isSelected })
                                    }
                                }
                                AuroraIcon {
                                    name: model.isDir ? "folder" : "file"
                                    iconSize: netDiskPage.fileIconSize
                                    color: model.isDir ? GTheme.primaryColor : GTheme.textSecondary
                                }
                                Label {
                                    text: model.fileName
                                    Layout.fillWidth: true
                                    color: GTheme.textPrimary
                                    font.pixelSize: GTheme.fontBody
                                    elide: Text.ElideMiddle
                                }
                                Label {
                                    text: model.fileSize
                                    visible: !netDiskPage.veryCompact
                                    Layout.preferredWidth: netDiskPage.compact ? 74 : 110
                                    color: GTheme.textSecondary
                                    font.pixelSize: GTheme.fontCaption
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: model.createTime
                                    visible: !netDiskPage.compact
                                    Layout.preferredWidth: 150
                                    color: GTheme.textSecondary
                                    font.pixelSize: GTheme.fontCaption
                                    elide: Text.ElideRight
                                }
                            }
                            MouseArea {
                                id: rowMouse
                                anchors.left: parent.left
                                anchors.leftMargin: netDiskPage.rowHeight
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                hoverEnabled: true
                                cursorShape: model.isDir ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: fileRow.openDirectory()
                            }
                            Keys.onReturnPressed: event => {
                                fileRow.openDirectory()
                                event.accepted = model.isDir
                            }
                            Keys.onEnterPressed: event => {
                                fileRow.openDirectory()
                                event.accepted = model.isDir
                            }
                            Keys.onSpacePressed: event => {
                                fileRow.openDirectory()
                                event.accepted = model.isDir
                            }
                            Component.onCompleted: if (netDiskPage.homePath.length === 0)
                                netDiskPage.homePath = Utils.getParentPath(model.filePath)
                        }
                    }

                    EmptyState {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - GTheme.spaceLG * 2, 360)
                        visible: netDiskPage.modelEmpty && !netDiskPage.isBusy
                        iconName: "cloud"
                        accentColor: GTheme.infoColor
                        title: qsTr("This folder is empty")
                        description: qsTr("Go back to another folder or parse a different share link.")
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM
            Text {
                text: selectAllCheckBox.checked ? qsTr("All visible files selected") : qsTr("Select files to add them to the download queue")
                color: selectAllCheckBox.checked ? GTheme.textInfo : GTheme.textSecondary
                font.pixelSize: GTheme.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: !netDiskPage.veryCompact
            }
            GButton {
                text: qsTr("Back")
                enabled: netDiskPage.parentPath.length > 0 && !netDiskPage.isBusy
                onClicked: {
                    let next = Utils.getParentPath(netDiskPage.parentPath)
                    if (next.length > 0 && next !== netDiskPage.parentPath) {
                        netDiskPage.parentPath = next
                        NetWorkDiskManager.ChangeDir(next, "")
                        netDiskPage.beginOperation()
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: netDiskPage.isBusy
        color: GTheme.bgWhite
        opacity: 0.92
        z: 10

        GCard {
            anchors.centerIn: parent
            width: Math.min(parent.width - GTheme.spaceLG * 2, 320)
            implicitHeight: busyContent.implicitHeight + GTheme.spaceLG * 2
            padding: GTheme.spaceLG
            variant: "elevated"
            shadow: true
            hoverEnabled: false
            ColumnLayout {
                id: busyContent
                anchors.fill: parent
                spacing: GTheme.spaceSM
                BusyIndicator { running: true; Layout.alignment: Qt.AlignHCenter }
                Text {
                    text: netDiskPage.parseMode ? qsTr("Parsing share link...") : qsTr("Loading folder...")
                    color: GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
                Text {
                    text: qsTr("This may take a moment for large shares.")
                    color: GTheme.textSecondary
                    font.pixelSize: GTheme.fontCaption
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Connections {
        id: netDiskConnections
        target: NetWorkDiskManager
        function onTaskFinished(msg, isSuccess, taskType) {
            operationTimeout.stop()
            netDiskPage.isBusy = false
            if (!isSuccess) {
                netDiskPage.lastErrorText = msg && msg.length > 0 ? msg : qsTr("The cloud request failed. Please try again.")
                ToastManager.ShowError(netDiskPage.lastErrorText)
                return
            }
            netDiskPage.lastErrorText = ""
            if (taskType === 0) {
                Qt.callLater(function() {
                    netDiskPage.parseMode = false
                    fileListView.forceLayout()
                })
            } else if (taskType === 1) {
                Qt.callLater(function() { fileListView.forceLayout() })
            }
        }
    }

    function checkShareUrl(url) {
        var reg = /https:\/\/pan\.baidu\.com\/s\/[A-Za-z0-9_-]+(\?pwd=[A-Za-z0-9]+)?/
        return reg.test(url)
    }
}
