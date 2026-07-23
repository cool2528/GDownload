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

    // MatchPlugins 返回的候选插件([{name, displayName, needsConfig, configured}])
    property var matchedPlugins: []
    property int selectedPluginIndex: 0
    property bool noPluginMatched: false
    readonly property var selectedPlugin: matchedPlugins.length > 0
                                          ? matchedPlugins[Math.min(selectedPluginIndex, matchedPlugins.length - 1)]
                                          : null
    readonly property bool selectedPluginNeedsSetup: selectedPlugin !== null
                                                     && selectedPlugin.needsConfig === true
                                                     && selectedPlugin.configured !== true

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

    function refreshMatches() {
        var url = Utils.removeNewlineAndTrim(urlInput.text)
        matchedPlugins = url.length > 0 ? NetWorkDiskManager.MatchPlugins(url) : []
        if (selectedPluginIndex >= matchedPlugins.length)
            selectedPluginIndex = 0
        if (matchedPlugins.length > 0 || url.length === 0)
            noPluginMatched = false
    }

    function parseShareLink() {
        var url = Utils.removeNewlineAndTrim(urlInput.text)
        refreshMatches()
        if (url.length === 0) {
            lastErrorText = qsTr("Enter a share link before parsing.")
            ToastManager.ShowError(lastErrorText)
            return
        }
        if (matchedPlugins.length === 0) {
            noPluginMatched = true
            lastErrorText = qsTr("No installed plugin can handle this link. Install one from the Plugin Market.")
            ToastManager.ShowError(lastErrorText)
            return
        }
        homePath = ""
        parentPath = ""
        NetWorkDiskManager.ParseShareUrl(url, matchedPlugins[selectedPluginIndex].name)
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
                        text: qsTr("Preview a cloud share before adding selected files to the queue.")
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
                        text: qsTr("Cloud share link")
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
                        placeholderText: qsTr("Paste a share link from a supported cloud drive here")
                        Accessible.name: qsTr("Cloud share link")
                        color: GTheme.textPrimary
                        placeholderTextColor: GTheme.textPlaceholder
                        background: Rectangle {
                            color: GTheme.fillLighter
                            radius: GTheme.radiusMedium
                            border.width: 1
                            border.color: netDiskPage.lastErrorText.length > 0 ? GTheme.dangerColor
                                                                                 : (urlInput.activeFocus ? GTheme.primaryColor : GTheme.borderLight)
                        }
                        onTextChanged: {
                            if (netDiskPage.lastErrorText.length > 0)
                                netDiskPage.lastErrorText = ""
                            netDiskPage.refreshMatches()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceSM
                        visible: netDiskPage.matchedPlugins.length > 1

                        Text {
                            text: qsTr("Parse with")
                            color: GTheme.textSecondary
                            font.pixelSize: GTheme.fontCaption
                        }
                        GComBoBox {
                            id: pluginSelector
                            objectName: "netDiskPluginSelector"
                            Layout.preferredWidth: 220
                            Layout.preferredHeight: GTheme.sizeDefault
                            model: netDiskPage.matchedPlugins.map(p => p.displayName)
                            currentIndex: netDiskPage.selectedPluginIndex
                            Accessible.name: qsTr("Plugin used for parsing")
                            onActivated: index => netDiskPage.selectedPluginIndex = index
                        }
                        Item { Layout.fillWidth: true }
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
                        { icon: "link", title: qsTr("Paste link"), description: qsTr("Auto-match an installed plugin for the link.") },
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
                visible: netDiskPage.selectedPlugin !== null
                severity: netDiskPage.selectedPluginNeedsSetup ? "warning" : "success"
                text: netDiskPage.selectedPlugin === null ? ""
                      : (netDiskPage.selectedPluginNeedsSetup
                         ? qsTr("%1 needs to be configured before parsing.").arg(netDiskPage.selectedPlugin.displayName)
                         : qsTr("%1 is ready. Parsed files remain local until you add them to the queue.").arg(netDiskPage.selectedPlugin.displayName))
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                Item { Layout.fillWidth: true }

                GButton {
                    visible: netDiskPage.noPluginMatched
                    text: qsTr("Open Plugin Market")
                    buttonType: "primary"
                    Layout.fillWidth: netDiskPage.veryCompact
                    onClicked: {
                        brower_view.index = 1
                        brower_view.switchSettingPage(3)
                    }
                }

                GButton {
                    visible: netDiskPage.selectedPluginNeedsSetup
                    text: qsTr("Configure plugin")
                    buttonType: "primary"
                    Layout.fillWidth: netDiskPage.veryCompact
                    onClicked: pluginSettingsDialog.openFor(netDiskPage.selectedPlugin.name)
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

    PluginSettingsDialog {
        id: pluginSettingsDialog
        parent: Overlay.overlay
    }

    VerificationDialog {
        id: verificationDialog
        parent: Overlay.overlay
        // 用户输码期间暂停 70s 操作超时,关闭后恢复计时
        onOpened: operationTimeout.stop()
        onClosed: {
            if (netDiskPage.isBusy)
                operationTimeout.restart()
        }
    }

    Connections {
        target: VerificationBridge
        // 插件请求验证输入:弹出对话框
        function onVerificationRequested(message, imageBase64) {
            verificationDialog.openFor(message, imageBase64)
        }
        // 桥接层等待超时:收起对话框
        function onRequestAborted() {
            verificationDialog.close()
        }
    }

    Connections {
        target: PluginConfigManager
        // 配置保存/清除后刷新候选插件的 configured 状态
        function onConfigChanged(name) {
            netDiskPage.refreshMatches()
        }
    }
}
