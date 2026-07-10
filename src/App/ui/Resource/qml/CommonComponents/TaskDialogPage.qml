import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格任务添加对话框
Popup {
    id: taskPage
    objectName: "taskDialogPage"
    width: dialogWidth
    implicitHeight: contentLayout.implicitHeight
    height: Math.min(parent ? parent.height - dialogViewportMargin * 2 : implicitHeight, implicitHeight)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    closePolicy: Popup.CloseOnEscape
    focus: true

    // 打开时预选的标签页:0=URL,1=Torrent,2=Baidu。
    // 供下载页空状态快捷入口(Add URL / Torrent / Baidu)按入口类型直达对应标签。
    property int initialTab: 0

    // 页面级布局常量:对话框宽度和内容高度只服务本弹窗,不是通用设计令牌
    readonly property int dialogWidth: 720
    readonly property int dialogViewportMargin: GTheme.space2XL
    readonly property int contentMinHeight: 460
    readonly property int urlPanelHeight: 120
    readonly property int torrentPanelHeight: 150
    readonly property int netDiskPanelHeight: 300
    readonly property int generalPanelHeight: 150
    readonly property int footerHeight: GTheme.titleBarHeight + GTheme.space2XL
    readonly property int actionButtonWidth: 100
    readonly property int cancelButtonWidth: 80

    // 视觉令牌别名:减少本文件重复,仍全部来自 GTheme
    readonly property int contentPadding: GTheme.space2XL
    readonly property int contentSpacing: GTheme.spaceSM
    readonly property int cardPadding: GTheme.spaceSM

    // Element Plus 风格背景
    background: Rectangle {
        color: GTheme.bgWhite
        radius: GTheme.radiusBase
        border.width: 1
        border.color: GTheme.borderLight

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationBase }
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
            // 最小高度取小值:窗口较矮时本区可收缩、内部滚动,
            // 保证底部按钮区(footer)始终留在弹窗内不被挤出窗口
            Layout.minimumHeight: GTheme.sizeLarge * 3
            Layout.preferredHeight: taskPage.contentMinHeight
            implicitHeight: taskPage.contentMinHeight
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            contentHeight: contentArea.height

            // 使用默认 Flickable 作为 contentItem，直接提供内容列
            ColumnLayout {
                id: contentArea
                width: scrollArea.availableWidth - taskPage.contentPadding * 2
                x: taskPage.contentPadding
                y: taskPage.contentPadding
                spacing: taskPage.contentSpacing

                // 标签页导航
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.navItemHeight + GTheme.spaceXS
                    color: "transparent"
                    objectName: "taskDialogTabs"

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
                QtObject { id: tabNavigation; property int currentIndex: taskPage.initialTab }

                // 标签页内容区域
                StackLayout {
                    id: taskPageLayout
                    objectName: "taskDialogStack"
                    Layout.fillWidth: true
                    Layout.preferredHeight: tabNavigation.currentIndex === 2
                                            ? taskPage.netDiskPanelHeight
                                            : (tabNavigation.currentIndex === 1
                                               ? taskPage.torrentPanelHeight
                                               : taskPage.urlPanelHeight)
                    currentIndex: tabNavigation.currentIndex
                    property int urlType: 0

                    // URL 输入页面
                    GCard {
                        outlined: true
                        padding: taskPage.cardPadding
                        Layout.preferredHeight: taskPage.urlPanelHeight
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0
                            ScrollView{
                                id:view_input
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                TextArea {
                                    id: input
                                    objectName: "inputUrl"
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    font.pixelSize: GTheme.fontBody
                                    placeholderText: qsTr("Enter download URLs (one per line, supports magnet links)")
                                    color: GTheme.textPrimary
                                    placeholderTextColor: GTheme.textPlaceholder
                                    selectByMouse: true
                                    wrapMode: TextArea.Wrap

                                    background: Rectangle {
                                        color: GTheme.fillLighter
                                        border.width: 1
                                        border.color: input.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                                        radius: GTheme.radiusBase

                                        Behavior on border.color {
                                            ColorAnimation { duration: GTheme.durationBase }
                                        }
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
                        padding: taskPage.cardPadding
                        Layout.preferredHeight: taskPage.torrentPanelHeight
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
                        padding: taskPage.cardPadding
                        Layout.fillHeight: true
                        Layout.preferredHeight: taskPage.netDiskPanelHeight
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
                            let urls = []
                            const lines = Utils.splitPath(input.text)
                            for (let i = 0; i < lines.length; ++i) {
                                const url = String(lines[i]).trim()
                                if (url.length > 0) {
                                    urls.push(url)
                                }
                            }
                            return urls
                        } else {
                            const path = String(dropTorrent.path || "").trim()
                            taskPageLayout.urlType = 1
                            let ext = path.split('.').pop().toLowerCase()
                            if (ext === "metalink" || ext === "meta4") {
                                taskPageLayout.urlType = 1
                            } else {
                                taskPageLayout.urlType = 2
                            }
                            return path
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
                    Layout.preferredHeight: taskPage.generalPanelHeight
                    standardSpacing: taskPage.contentSpacing
                    visible: tabNavigation.currentIndex !== 2
                }

                // 高级配置区域
                TaskAdvancedOptionsCard {
                    id: additionalConfig
                    standardSpacing: taskPage.contentSpacing
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: advanced.checked && tabNavigation.currentIndex !== 2 ? GTheme.spaceSM : 0
                    visible: advanced.checked && tabNavigation.currentIndex !== 2
                    Layout.preferredHeight: visible ? additionalConfig.view.implicitHeight + GTheme.sizeLarge + taskPage.contentSpacing : 0
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
            id: footerArea
            objectName: "taskDialogFooter"
            Layout.fillWidth: true
            Layout.preferredHeight: taskPage.footerHeight
            color: GTheme.bgWhite

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: taskPage.contentPadding
                anchors.rightMargin: taskPage.contentPadding
                spacing: taskPage.contentSpacing

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
                    Layout.preferredWidth: taskPage.cancelButtonWidth
                    Layout.preferredHeight: GTheme.sizeDefault
                    onClicked: taskPage.close()
                }

                // 提交按钮
                GButton {
                    type: 1  // Primary
                    objectName: "btnCreateTask"
                    text: qsTr("Add Task")
                    Layout.preferredWidth: taskPage.actionButtonWidth
                    Layout.preferredHeight: GTheme.sizeDefault
                    onClicked: {
                        let taskAdded = true
                        if (tabNavigation.currentIndex !== 2) {
                            taskAdded = false
                            let url = taskPageLayout.geturls()
                            let options = taskPageLayout.getOptions()
                            if (taskPageLayout.urlType === 0) {
                                if (url.length === 0) {
                                    ToastManager.ShowError(qsTr("Please enter at least one download URL."))
                                    return
                                }
                                taskAdded = BrowserManager.AddHttpTask(url, options)
                            } else if (taskPageLayout.urlType === 1) {
                                if (url.length === 0) {
                                    ToastManager.ShowError(qsTr("Please select a Torrent or Metalink file."))
                                    return
                                }
                                taskAdded = BrowserManager.AddMetalinkTask(url, options)
                            } else if (taskPageLayout.urlType === 2) {
                                if (url.length === 0) {
                                    ToastManager.ShowError(qsTr("Please select a Torrent or Metalink file."))
                                    return
                                }
                                taskAdded = BrowserManager.AddTorrentTask(url, options)
                            }
                        } else {
                            NetWorkDiskManager.DownloadSelectedFiles()
                        }
                        if (!taskAdded) {
                            return
                        }
                        if (tabNavigation.currentIndex !== 2) {
                            ToastManager.ShowSuccess(qsTr("Download task added successfully."))
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
                duration: GTheme.durationSlow
                easing.type: GTheme.easingStandard
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: GTheme.durationSlow
                easing.type: GTheme.easingStandard
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
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
        }
    }
}
