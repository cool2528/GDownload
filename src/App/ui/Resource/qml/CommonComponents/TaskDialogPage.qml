import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格任务添加对话框
Popup {
    id: taskPage
    objectName: "taskDialogPage"
    width: Math.min(dialogWidth, parent ? Math.max(320, parent.width - dialogViewportMargin * 2) : dialogWidth)
    implicitHeight: contentLayout.implicitHeight
    height: Math.min(parent ? parent.height - dialogViewportMargin * 2 : implicitHeight, implicitHeight)
    x: parent ? Math.max(0, Math.round((parent.width - width) / 2)) : 0
    y: parent ? Math.max(0, Math.round((parent.height - height) / 2)) : 0
    modal: true
    closePolicy: Popup.CloseOnEscape
    focus: true

    // 打开时预选的标签页:0=URL,1=Torrent,2=eD2k,3=Cloud Drive。
    // 供下载页空状态快捷入口(Add URL / Torrent / Cloud Drive)按入口类型直达对应标签。
    property int initialTab: 0
    property string validationMessage: ""

    // 页面级布局常量:对话框宽度和内容高度只服务本弹窗,不是通用设计令牌
    readonly property int dialogWidth: 720
    readonly property int dialogViewportMargin: GTheme.space2XL
    readonly property int contentMinHeight: 460
    readonly property int urlPanelHeight: 120
    readonly property int torrentPanelHeight: 150
    readonly property int ed2kPanelHeight: 300
    readonly property int netDiskPanelHeight: 300
    readonly property bool compactDialog: width < 560
    readonly property int generalPanelHeight: compactDialog ? 330 : 150
    readonly property int footerHeight: compactDialog
                                        ? GTheme.sizeDefault * 2 + GTheme.spaceLG * 3
                                        : GTheme.titleBarHeight + GTheme.space2XL
    readonly property int actionButtonWidth: 100
    readonly property int cancelButtonWidth: 80

    // 视觉令牌别名:减少本文件重复,仍全部来自 GTheme
    readonly property int contentPadding: compactDialog ? GTheme.spaceLG : GTheme.space2XL
    readonly property int contentSpacing: GTheme.spaceSM
    readonly property int cardPadding: GTheme.spaceMD

    // Element Plus 风格背景
    background: Rectangle {
        color: GTheme.surfaceElevated
        radius: GTheme.radiusLarge
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
            objectName: "taskDialogScroll"
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
                        anchors.leftMargin: GTheme.spaceSM
                        anchors.rightMargin: GTheme.spaceSM
                        spacing: 0

                        Repeater {
                            id: tabRepeater
                            model: [
                                { name: qsTr("URL"), iconName: "link" },
                                { name: qsTr("Torrent"), iconName: "cloud-download" },
                                { name: qsTr("eD2k"), iconName: "connected" },
                                { name: qsTr("Cloud Drive"), iconName: "cloud" }
                            ]

                            GButton {
                                variant: "nav"
                                objectName: "taskSourceTab" + index
                                required property int index
                                required property var modelData

                                Layout.fillWidth: true
                                Layout.preferredHeight: GTheme.navItemHeight
                                checkable: true
                                checked: index === tabNavigation.currentIndex
                                iconName: modelData.iconName
                                text: modelData.name
                                Accessible.name: modelData.name
                                ButtonGroup.group: tabGroup
                                onClicked: {
                                    tabNavigation.currentIndex = index
                                    taskPage.validationMessage = ""
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
                    Layout.preferredHeight: tabNavigation.currentIndex === 3
                                            ? taskPage.netDiskPanelHeight
                                            : (tabNavigation.currentIndex === 2
                                               ? taskPage.ed2kPanelHeight
                                               : (tabNavigation.currentIndex === 1
                                                  ? taskPage.torrentPanelHeight
                                                  : taskPage.urlPanelHeight))
                    currentIndex: tabNavigation.currentIndex
                    property int urlType: 0
                    // eD2k tab 专用：previewModel 只携带文件名/大小等展示信息，不保留原始链接，
                    // 这里按解析文本的顺序另存一份原始链接，供提交时用勾选的 1-based 索引反查完整链接
                    property var ed2kLinks: []

                    // URL 输入页面
                        GCard {
                            outlined: true
                            padding: taskPage.cardPadding
                            Layout.preferredHeight: taskPage.urlPanelHeight
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: taskPage.cardPadding
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
                                    activeFocusOnTab: enabled && visible
                                    Accessible.name: qsTr("Download URLs")
                                    onTextChanged: {
                                        if (text.trim().length > 0)
                                            taskPage.validationMessage = ""
                                    }

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
                                objectName: "taskTorrentDropArea"
                                anchors.fill: parent
                                anchors.margins: taskPage.cardPadding
                                visible: true
                            onAccepted: {
                                let model = BrowserManager.GetFilePreviewModel(dropTorrent.path)
                                if (model) {
                                    dropTorrent.visible = false
                                    filePreview.previewModel = model
                                    taskPage.validationMessage = ""
                                }
                            }
                        }

                            FilePreviewList {
                                id: filePreview
                                objectName: "taskTorrentFilePreview"
                                anchors.fill: parent
                                anchors.margins: taskPage.cardPadding
                                visible: !dropTorrent.visible
                            onClearRequested: {
                                filePreview.previewModel.clear()
                                filePreview.previewModel = null
                                dropTorrent.visible = true
                            }
                        }
                    }

                    // eD2k 链接页面：粘贴 ed2k://|file|...| 链接，实时解析出文件列表供勾选
                        GCard {
                            outlined: true
                            padding: taskPage.cardPadding
                            Layout.preferredHeight: taskPage.ed2kPanelHeight
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: taskPage.cardPadding
                                spacing: taskPage.contentSpacing

                            ScrollView {
                                id: view_ed2kInput
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.round(taskPage.ed2kPanelHeight * 0.35)
                                TextArea {
                                    id: ed2kInput
                                    objectName: "inputEd2k"
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    font.pixelSize: GTheme.fontBody
                                    placeholderText: qsTr("Enter ed2k links (one per line)")
                                    color: GTheme.textPrimary
                                    placeholderTextColor: GTheme.textPlaceholder
                                    selectByMouse: true
                                    wrapMode: TextArea.Wrap
                                    activeFocusOnTab: enabled && visible
                                    Accessible.name: qsTr("eD2k Links")
                                    onTextChanged: {
                                        taskPage.validationMessage = ""
                                        ed2kParseTimer.restart()
                                    }

                                    background: Rectangle {
                                        color: GTheme.fillLighter
                                        border.width: 1
                                        border.color: ed2kInput.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                                        radius: GTheme.radiusBase

                                        Behavior on border.color {
                                            ColorAnimation { duration: GTheme.durationBase }
                                        }
                                    }

                                    Component.onCompleted: {
                                        ed2kInput.text = ClipboardWatcher.GetClipboardText()
                                    }
                                    // 用 Connections 而非匿名 connect：组件销毁时自动断开，
                                    // 避免对话框关闭后仍向已销毁的 ed2kInput 写入（UAF）。
                                    Connections {
                                        target: ClipboardWatcher
                                        function onClipboardChanged(text) {
                                            if (taskPage.visible && text.length > 3) {
                                                ed2kInput.text = text
                                            }
                                        }
                                    }
                                }
                            }

                            // 防抖：避免粘贴/输入过程中每个字符都触发一次模型重建
                            Timer {
                                id: ed2kParseTimer
                                interval: 300
                                repeat: false
                                onTriggered: {
                                    const text = ed2kInput.text
                                    if (text.trim().length === 0) {
                                        if (ed2kPreview.previewModel) {
                                            ed2kPreview.previewModel.clear()
                                        }
                                        ed2kPreview.previewModel = null
                                        taskPageLayout.ed2kLinks = []
                                    } else {
                                        // 链接列表与预览模型必须来自同一 C++ 解析器，保证行索引一一对应
                                        taskPageLayout.ed2kLinks = BrowserManager.GetValidEd2kLinks(text)
                                        ed2kPreview.previewModel = BrowserManager.ParseEd2kLinks(text)
                                    }
                                }
                            }

                            FilePreviewList {
                                id: ed2kPreview
                                objectName: "taskEd2kFilePreview"
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }

                    // 网盘解析页面(插件通用)
                        GCard {
                            outlined: true
                            padding: taskPage.cardPadding
                            Layout.fillHeight: true
                            Layout.preferredHeight: taskPage.netDiskPanelHeight
                            NetDiskPageView {
                                id: netDiskPageView
                            anchors.fill: parent
                            anchors.margins: taskPage.cardPadding
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
                        } else if (currentIndex === 1) {
                            const path = String(dropTorrent.path || "").trim()
                            taskPageLayout.urlType = 1
                            let ext = path.split('.').pop().toLowerCase()
                            if (ext === "metalink" || ext === "meta4") {
                                taskPageLayout.urlType = 1
                            } else {
                                taskPageLayout.urlType = 2
                            }
                            return path
                        } else {
                            // eD2k：previewModel.getSelectedFiles() 返回的是勾选行的 1-based 索引字符串
                            // （与 torrent 的 select-file 语义一致），这里映射回 ed2kLinks 里的原始链接
                            taskPageLayout.urlType = 3
                            let selected = []
                            if (ed2kPreview.previewModel) {
                                const indices = ed2kPreview.previewModel.getSelectedFiles()
                                for (let i = 0; i < indices.length; ++i) {
                                    const idx = parseInt(indices[i], 10) - 1
                                    if (idx >= 0 && idx < taskPageLayout.ed2kLinks.length) {
                                        selected.push(taskPageLayout.ed2kLinks[idx])
                                    }
                                }
                            }
                            return selected
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
                        if (currentIndex === 0) {
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
                            const customHeaders = additionalConfig.collectRequestHeaders()
                            if (customHeaders.length > 0) {
                                headers = headers.concat(customHeaders)
                            }
                            if (headers.length > 0) {
                                options["header"] = headers
                            }
                        }
                        if (currentIndex === 1) {
                            // previewModel 初始为 null（未拖入 torrent 时），直接访问会空指针崩溃
                            if (filePreview.previewModel) {
                                let select_files = filePreview.previewModel.getSelectedFiles()
                                if (select_files && select_files.length > 0) {
                                    options["select-file"] = select_files.join()
                                }
                            }
                            // Metalink 与 Torrent 共用导入标签，但 BT 任务选项只传给 addTorrent。
                            if (taskPageLayout.urlType === 2 && advanced.checked) {
                                const torrentOptions = additionalConfig.collectTorrentOptions()
                                for (const key in torrentOptions)
                                    options[key] = torrentOptions[key]
                            }
                        }
                        return options
                    }
                }

                AlertTip {
                    objectName: "taskDialogValidationAlert"
                    Layout.fillWidth: true
                    visible: taskPage.validationMessage.length > 0
                    severity: "danger"
                    title: qsTr("Check the task details")
                    description: taskPage.validationMessage
                    showClose: true
                    onCloseRequested: taskPage.validationMessage = ""
                }

                // 基础配置区域
                TaskGeneralOptionsCard {
                    id: generalConfig
                    Layout.fillWidth: true
                    Layout.preferredHeight: taskPage.generalPanelHeight
                    standardSpacing: taskPage.contentSpacing
                    visible: tabNavigation.currentIndex !== 3
                }

                // 高级配置区域：eD2k(2) 与 Cloud Drive(3) 都没有对应的 HTTP/Torrent 高级选项，隐藏
                TaskAdvancedOptionsCard {
                    id: additionalConfig
                    optionMode: tabNavigation.currentIndex === 1 ? "torrent" : "http"
                    standardSpacing: taskPage.contentSpacing
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: advanced.checked && tabNavigation.currentIndex !== 2 && tabNavigation.currentIndex !== 3 ? GTheme.spaceSM : 0
                    visible: advanced.checked && tabNavigation.currentIndex !== 2 && tabNavigation.currentIndex !== 3
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
            color: GTheme.surfaceElevated

            GridLayout {
                anchors.fill: parent
                anchors.leftMargin: taskPage.contentPadding
                anchors.rightMargin: taskPage.contentPadding
                anchors.topMargin: taskPage.compactDialog ? GTheme.spaceLG : 0
                anchors.bottomMargin: taskPage.compactDialog ? GTheme.spaceLG : 0
                columns: taskPage.compactDialog ? 2 : 4
                rowSpacing: GTheme.spaceSM
                columnSpacing: taskPage.contentSpacing

                // 高级选项开关
                GCheckBox {
                    id: advanced
                    objectName: "taskAdvancedOptionsToggle"
                    text: qsTr("Advanced Options")
                    visible: tabNavigation.currentIndex !== 2 && tabNavigation.currentIndex !== 3
                    Layout.alignment: Qt.AlignVCenter
                    Layout.columnSpan: taskPage.compactDialog ? 2 : 1

                }

                Item {
                    Layout.fillWidth: true
                    visible: !taskPage.compactDialog
                    Layout.columnSpan: advanced.visible ? 1 : 2
                }

                // 取消按钮
                GButton {
                    objectName: "btnCancelTask"
                    text: qsTr("Cancel")
                    Layout.fillWidth: taskPage.compactDialog
                    Layout.preferredWidth: taskPage.compactDialog ? -1 : taskPage.cancelButtonWidth
                    Layout.preferredHeight: GTheme.sizeDefault
                    onClicked: taskPage.close()
                }

                // 提交按钮
                GButton {
                    type: 1  // Primary
                    objectName: "btnCreateTask"
                    text: qsTr("Add Task")
                    Layout.fillWidth: taskPage.compactDialog
                    Layout.preferredWidth: taskPage.compactDialog ? -1 : taskPage.actionButtonWidth
                    Layout.preferredHeight: GTheme.sizeDefault
                    onClicked: {
                        let taskAdded = true
                        if (tabNavigation.currentIndex === 3) {
                            // Cloud Drive：由插件层触发批量下载，不经过 aria2/ed2k 任务体系
                            NetWorkDiskManager.DownloadSelectedFiles()
                        } else if (tabNavigation.currentIndex === 2) {
                            // eD2k：提交勾选的原始链接列表，交给独立的 Ed2kDownloadManager
                            const links = taskPageLayout.geturls()
                            if (links.length === 0) {
                                taskPage.validationMessage = qsTr("Please paste at least one eD2k link and select a file.")
                                ToastManager.ShowError(taskPage.validationMessage)
                                return
                            }
                            const options = taskPageLayout.getOptions()
                            taskAdded = BrowserManager.AddEd2kTask(links, options)
                        } else {
                            taskAdded = false
                            let url = taskPageLayout.geturls()
                            if (tabNavigation.currentIndex === 1 &&
                                    taskPageLayout.urlType === 2 && advanced.checked) {
                                const optionError = additionalConfig.torrentValidationMessage()
                                if (optionError.length > 0) {
                                    taskPage.validationMessage = optionError
                                    ToastManager.ShowError(taskPage.validationMessage)
                                    return
                                }
                            }
                            let options = taskPageLayout.getOptions()
                            if (taskPageLayout.urlType === 0) {
                                if (url.length === 0) {
                                    taskPage.validationMessage = qsTr("Please enter at least one download URL.")
                                    ToastManager.ShowError(taskPage.validationMessage)
                                    return
                                }
                                taskAdded = BrowserManager.AddHttpTask(url, options)
                            } else if (taskPageLayout.urlType === 1) {
                                if (url.length === 0) {
                                    taskPage.validationMessage = qsTr("Please select a Torrent or Metalink file.")
                                    ToastManager.ShowError(taskPage.validationMessage)
                                    return
                                }
                                taskAdded = BrowserManager.AddMetalinkTask(url, options)
                            } else if (taskPageLayout.urlType === 2) {
                                if (url.length === 0) {
                                    taskPage.validationMessage = qsTr("Please select a Torrent or Metalink file.")
                                    ToastManager.ShowError(taskPage.validationMessage)
                                    return
                                }
                                taskAdded = BrowserManager.AddTorrentTask(url, options)
                            }
                        }
                        if (!taskAdded) {
                            if (tabNavigation.currentIndex === 2) {
                                taskPage.validationMessage = qsTr("Failed to add eD2k task. Please check the link(s).")
                                ToastManager.ShowError(taskPage.validationMessage)
                            }
                            return
                        }
                        if (tabNavigation.currentIndex !== 3) {
                            ToastManager.ShowSuccess(qsTr("Download task added successfully."))
                        }
                        brower_view.index = 0
                        brower_view.switchDownloadPage(0)
                        if (tabNavigation.currentIndex !== 3) {
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
