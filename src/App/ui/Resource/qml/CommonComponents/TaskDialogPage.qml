import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格任务添加对话框
Popup {
    id: taskPage
    width: 720
    height: Math.min(1000, contentLayout.implicitHeight + standardPadding * 2)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    focus: true

    // Element Plus 设计标准
    readonly property int standardPadding: 24
    readonly property int standardSpacing: 16
    readonly property int headerHeight: 64
    readonly property int buttonHeight: 32

    // Element Plus 风格背景
    background: Rectangle {
        color: GTheme.bgWhite
        radius: 8
        border.width: 1
        border.color: GTheme.borderLight

        // Element Plus 风格阴影
        layer.enabled: true
        layer.effect: DropShadow {
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.1)
            horizontalOffset: 0
            verticalOffset: 4
        }
    }

    contentItem: ColumnLayout {
        id: contentLayout
        spacing: 0

        // 头部区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: taskPage.headerHeight
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: taskPage.standardPadding
                anchors.rightMargin: taskPage.standardPadding
                spacing: taskPage.standardSpacing

                // 图标区域
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    Layout.alignment: Qt.AlignVCenter
                    color: GTheme.primaryLight(9)
                    radius: 8

                    FontIcon {
                        anchors.centerIn: parent
                        iconSource: SegoeFluentIcons.Add
                        iconSize: 20
                        color: GTheme.primaryColor
                    }
                }

                // 标题信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("Add New Download Task")
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: GTheme.textPrimary
                    }

                    Text {
                        text: qsTr("Add downloads from URLs, torrents, or cloud storage")
                        font.pixelSize: 14
                        color: GTheme.textSecondary
                    }
                }

                // 关闭按钮
                IconButton {
                    iconSource: SegoeFluentIcons.ChromeClose
                    iconSize: 14
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    iconColor: hovered ? GTheme.textPrimary : GTheme.textSecondary
                    backgroundColor: hovered ? GTheme.fillLight : "transparent"
                    onClicked: taskPage.close()
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 主要内容区域
        Item {
            Layout.fillWidth: true
            implicitHeight: Math.max(460, contentArea.implicitHeight)
            Layout.preferredHeight: implicitHeight

            ColumnLayout {
                id: contentArea
                anchors.fill: parent
                anchors.margins: taskPage.standardPadding
                spacing: taskPage.standardSpacing

                // 标签页导航
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Repeater {
                            id: tabRepeater
                            model: [
                                { name: qsTr("URL"), icon: SegoeFluentIcons.Link },
                                { name: qsTr("Torrent"), icon: SegoeFluentIcons.CloudDownload },
                                { name: qsTr("Baidu"), icon: SegoeFluentIcons.CloudFolder }
                            ]

                            GNavButton {
                                required property int index
                                required property var modelData

                                Layout.fillWidth: true
                                Layout.preferredHeight: 44
                                checkable: true
                                checked: index === tabNavigation.currentIndex
                                iconSource: modelData.icon
                                text: modelData.name
                                ButtonGroup.group: tabGroup
                                onClicked: {
                                    tabNavigation.currentIndex = index
                                }
                            }
                        }
                    }

                    ButtonGroup {
                        id: tabGroup
                    }

                    QtObject {
                        id: tabNavigation
                        property int currentIndex: 0
                    }
                }

                // 标签页内容区域
                StackLayout {
                    id: taskPageLayout
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    currentIndex: tabNavigation.currentIndex
                    property int urlType: 0

                    // URL 输入页面
                    GCard {
                        outlined: true
                        padding: taskPage.standardSpacing

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Text {
                                text: qsTr("Download URLs")
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: GTheme.textPrimary
                            }

                            TextArea {
                                id: input
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 120
                                font.pixelSize: 13
                                placeholderText: qsTr("Enter download URLs (one per line, supports magnet links)")
                                color: GTheme.textPrimary
                                placeholderTextColor: GTheme.textPlaceholder
                                selectByMouse: true
                                wrapMode: TextArea.Wrap

                                background: Rectangle {
                                    color: GTheme.fillLighter
                                    border.width: 1
                                    border.color: input.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                                    radius: 6

                                    Behavior on border.color {
                                        ColorAnimation { duration: 150 }
                                    }
                                }

                                Component.onCompleted: {
                                    ClipboardWatcher.clipboardChanged.connect(function(text) {
                                        if (text.length > 3) {
                                            input.text = text
                                        }
                                    })
                                    input.text = ClipboardWatcher.GetClipboardText()
                                }
                            }
                        }
                    }

                    // Torrent 文件页面
                    GCard {
                        outlined: true
                        padding: taskPage.standardSpacing

                        GDropArea {
                            id: dropTorrent
                            anchors.fill: parent
                            visible: true

                            onAccepted: {
                                let model = BrowserManager.GetFilePreviewModel(dropTorrent.path)
                                if (model) {
                                    dropTorrent.visible = false
                                    filePreview.previewModel = model
                                }
                            }
                        }

                        FilePreviewList {
                            id: filePreview
                            anchors.fill: parent
                            visible: !dropTorrent.visible
                            onClearRequested: {
                                filePreview.previewModel.clear()
                                filePreview.previewModel = null
                                dropTorrent.visible = true
                            }
                        }
                    }

                    // 百度网盘页面
                    GCard {
                        outlined: true
                        padding: taskPage.standardSpacing

                        NetDiskPageView {
                            id: netDiskPageView
                            anchors.fill: parent
                        }
                    }

                    // 辅助函数
                    function geturls() {
                        if (currentIndex === 0) {
                            taskPageLayout.urlType = 0
                            return Utils.splitPath(input.text)
                        } else {
                            taskPageLayout.urlType = 1
                            let ext = dropTorrent.path.split('.').pop()
                            if (ext === "metalink" || ext === "meta4") {
                                taskPageLayout.urlType = 1
                            } else {
                                taskPageLayout.urlType = 2
                            }
                            return dropTorrent.path
                        }
                    }

                    function getOptions() {
                        let options = {}
                        let headers = []
                        if (renameEdit.text.length > 0) {
                            options["out"] = renameEdit.text
                        }
                        if (spinbox.value > 0) {
                            options["max-concurrent-downloads"] = String("%1").arg(spinbox.value)
                        }
                        if (savePath.path.length > 0) {
                            options["dir"] = savePath.path
                        }
                        if (userAgent.text.length > 0) {
                            options["user-agent"] = userAgent.text
                        }
                        if (authorization.text.length > 0) {
                            options["http-auth-challenge"] = "true"
                            headers.push(String("Authorization: %1").arg(authorization.text))
                        }
                        if (cookie.text.length > 0) {
                            headers.push(String("Cookie: %1").arg(cookie.text))
                        }
                        if (referrer.text.length > 0) {
                            options["referer"] = referrer.text
                        }
                        if (currentIndex === 1) {
                            let select_files = filePreview.previewModel.getSelectedFiles()
                            options["select-file"] = select_files.join()
                        }
                        let customHeaders = customRequestHeaderList.getRequestHeaderList()
                        if (customHeaders.length > 0) {
                            headers = headers.concat(customHeaders)
                        }
                        if (headers.length > 0) {
                            options["header"] = headers
                        }
                        return options
                    }
                }

                // 基础配置区域
                GCard {
                    id: generalConfig
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    visible: tabNavigation.currentIndex !== 2
                    outlined: true
                    padding: taskPage.standardSpacing

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: taskPage.standardSpacing

                        Text {
                            text: qsTr("Download Settings")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: GTheme.textPrimary
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 4
                            columnSpacing: taskPage.standardSpacing
                            rowSpacing: 12

                            // 重命名
                            Text {
                                text: qsTr("Rename:")
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter
                            }

                            GTextField {
                                id: renameEdit
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: qsTr("Optional filename")
                            }

                            // 分片数
                            Text {
                                text: qsTr("Splits:")
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter
                            }

                            GSpinBox {
                                id: spinbox
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 32
                                from: 1
                                to: 64
                                value: 16
                            }

                            // 保存路径
                            Text {
                                text: qsTr("Save to:")
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter
                            }

                            FolderSelector {
                                id: savePath
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                Layout.columnSpan: 3
                                path: SettingsManager.qDir
                            }
                        }
                    }
                }

                // 高级配置区域
                GCard {
                    id: additionalConfig
                    Layout.fillWidth: true
                    Layout.topMargin: advanced.checked && tabNavigation.currentIndex !== 2 ? 16 : 0
                    visible: advanced.checked && tabNavigation.currentIndex !== 2
                    outlined: true
                    padding: taskPage.standardSpacing
                    Layout.preferredHeight: visible ? advancedContent.implicitHeight + padding * 2 : 0
                    Layout.minimumHeight: Layout.preferredHeight

                    ColumnLayout {
                        id: advancedContent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        spacing: taskPage.standardSpacing

                        Text {
                            text: qsTr("Advanced Options")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: GTheme.textPrimary
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: taskPage.standardSpacing
                            rowSpacing: 12

                            Text {
                                text: "User-Agent:"
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                Layout.preferredWidth: 100
                            }
                            GTextField {
                                id: userAgent
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "User-Agent"
                            }

                            Text {
                                text: "Authorization:"
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                Layout.preferredWidth: 100
                            }
                            GTextField {
                                id: authorization
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "Authorization"
                            }

                            Text {
                                text: "Referer:"
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                Layout.preferredWidth: 100
                            }
                            GTextField {
                                id: referrer
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "Referer"
                            }

                            Text {
                                text: "Cookie:"
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                Layout.preferredWidth: 100
                            }
                            GTextField {
                                id: cookie
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "Cookie"
                            }

                            Text {
                                text: qsTr("Custom Headers:")
                                font.pixelSize: 13
                                color: GTheme.textSecondary
                                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                                Layout.preferredWidth: 100
                            }

                            TextArea {
                                id: customRequestHeaderList
                                Layout.fillWidth: true
                                Layout.preferredHeight: 80
                                font.pixelSize: 12
                                placeholderText: qsTr("Custom request headers (one per line: KEY:VALUE)")
                                color: GTheme.textPrimary
                                placeholderTextColor: GTheme.textPlaceholder
                                selectByMouse: true
                                wrapMode: TextArea.Wrap

                                background: Rectangle {
                                    color: GTheme.fillLighter
                                    border.width: 1
                                    border.color: customRequestHeaderList.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                                    radius: 6

                                    Behavior on border.color {
                                        ColorAnimation { duration: 150 }
                                    }
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
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 底部按钮区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: taskPage.headerHeight
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: taskPage.standardPadding
                anchors.rightMargin: taskPage.standardPadding
                spacing: taskPage.standardSpacing

                // 高级选项开关
                GCheckBox {
                    id: advanced
                    text: qsTr("Advanced Options")
                    visible: tabNavigation.currentIndex !== 2
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                // 取消按钮
                GButton {
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: taskPage.buttonHeight
                    onClicked: taskPage.close()
                }

                // 提交按钮
                GButton {
                    type: 1  // Primary
                    text: qsTr("Add Task")
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: taskPage.buttonHeight
                    onClicked: {
                        if (tabNavigation.currentIndex !== 2) {
                            let url = taskPageLayout.geturls()
                            let options = taskPageLayout.getOptions()
                            if (taskPageLayout.urlType === 0) {
                                BrowserManager.AddHttpTask(url, options)
                            } else if (taskPageLayout.urlType === 1) {
                                BrowserManager.AddMetalinkTask(url, options)
                            } else if (taskPageLayout.urlType === 2) {
                                BrowserManager.AddTorrentTask(url, options)
                            }
                        } else {
                            NetWorkDiskManager.DownloadSelectedFiles()
                        }
                        brower_view.index = 0
                        brower_view.switchDownloadPage(0)
                        if (tabNavigation.currentIndex !== 2) {
                            taskPage.close()
                        }
                    }
                }
            }
        }

    }
    // 打开动画
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    // 关闭动画
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }
}
