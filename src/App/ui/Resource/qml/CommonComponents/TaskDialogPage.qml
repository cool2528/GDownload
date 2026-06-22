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
    implicitHeight: contentLayout.implicitHeight + standardPadding * 2
    height: Math.min(parent ? parent.height - standardPadding * 2 : 1000, implicitHeight)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    focus: true

    // Element Plus 设计标准
    readonly property int standardPadding: 24
    readonly property int standardSpacing: 5
    readonly property int headerHeight: 64
    readonly property int buttonHeight: 32
    readonly property int contentMinHeight: 460

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
        TaskDialogHeader {
            onCloseRequested: taskPage.close()
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 主要内容区域
        ScrollView {
            id: scrollArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: taskPage.contentMinHeight
            Layout.preferredHeight: taskPage.contentMinHeight
            implicitHeight: taskPage.contentMinHeight
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            contentHeight: contentArea.height

            // 使用默认 Flickable 作为 contentItem，直接提供内容列
            ColumnLayout {
                id: contentArea
                width: scrollArea.availableWidth - taskPage.standardPadding * 2
                x: taskPage.standardPadding
                y: taskPage.standardPadding
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
                                { name: qsTr("Baidu"), icon: SegoeFluentIcons.Cloud }
                            ]

                            GButton {
                                variant: "nav"
                                required property int index
                                required property var modelData

                                Layout.fillWidth: true
                                Layout.preferredHeight: GTheme.navItemHeight
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
                }

                ButtonGroup { id: tabGroup }
                QtObject { id: tabNavigation; property int currentIndex: 0 }

                // 标签页内容区域
                StackLayout {
                    id: taskPageLayout
                    Layout.fillWidth: true
                    // 自适应当前页内容高度，避免固定 200 导致可视区过小
                    //Layout.preferredHeight: 300
                    Layout.fillHeight: true
                    currentIndex: tabNavigation.currentIndex
                    property int urlType: 0

                    // URL 输入页面
                    GCard {
                        outlined: true
                        padding: taskPage.standardSpacing
                        Layout.preferredHeight: 150
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0
                            ScrollView{
                                id:view_input
                                anchors.fill: parent
                                TextArea {
                                    id: input
                                    objectName: "inputUrl"
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
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

                                        Behavior on border.color { ColorAnimation { duration: 150 } }
                                    }

                                    Component.onCompleted: {
                                        input.text = ClipboardWatcher.GetClipboardText()
                                    }
                                    // 用 Connections 而非匿名 connect：组件销毁时自动断开，
                                    // 避免对话框关闭后仍向已销毁的 input 写入（UAF）。
                                    Connections {
                                        target: ClipboardWatcher
                                        function onClipboardChanged(text) {
                                            if (taskPage.visible && text.length > 3) {
                                                input.text = text
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Torrent 文件页面
                    GCard {
                        outlined: true
                        padding: taskPage.standardSpacing
                        Layout.preferredHeight: 150
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
                        Layout.fillHeight: true
                        Layout.preferredHeight: 300
                        NetDiskPageView {
                            id: netDiskPageView
                            anchors.fill: parent
                            // NetDiskPageView 根为 Rectangle，默认无隐式高度，提供一个合理的缺省高度
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
                        if (generalConfig.renameText.length > 0) {
                            options["out"] = generalConfig.renameText
                        }
                        if (generalConfig.splitsValue > 0) {
                            options["split"] = String("%1").arg(generalConfig.splitsValue)
                        }
                        if (generalConfig.saveDirectory.length > 0) {
                            options["dir"] = generalConfig.saveDirectory
                        }
                        if (additionalConfig.userAgentText.length > 0) {
                            options["user-agent"] = additionalConfig.userAgentText
                        }
                        if (additionalConfig.authorizationText.length > 0) {
                            options["http-auth-challenge"] = "true"
                            headers.push(String("Authorization: %1").arg(additionalConfig.authorizationText))
                        }
                        if (additionalConfig.cookieText.length > 0) {
                            headers.push(String("Cookie: %1").arg(additionalConfig.cookieText))
                        }
                        if (additionalConfig.referrerText.length > 0) {
                            options["referer"] = additionalConfig.referrerText
                        }
                        if (currentIndex === 1) {
                            // previewModel 初始为 null（未拖入 torrent 时），直接访问会空指针崩溃
                            if (filePreview.previewModel) {
                                let select_files = filePreview.previewModel.getSelectedFiles()
                                if (select_files && select_files.length > 0) {
                                    options["select-file"] = select_files.join()
                                }
                            }
                        }
                        let customHeaders = additionalConfig.collectRequestHeaders()
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
                TaskGeneralOptionsCard {
                    id: generalConfig
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    standardSpacing: taskPage.standardSpacing
                    visible: tabNavigation.currentIndex !== 2
                }

                // 高级配置区域
                TaskAdvancedOptionsCard {
                    id: additionalConfig
                    standardSpacing: taskPage.standardSpacing
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: advanced.checked && tabNavigation.currentIndex !== 2 ? 8 : 0
                    visible: advanced.checked && tabNavigation.currentIndex !== 2
                    Layout.preferredHeight: visible ? additionalConfig.view.implicitHeight + 50 + taskPage.standardSpacing : 0
                    Layout.minimumHeight: Layout.preferredHeight
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
                    objectName: "btnCreateTask"
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
