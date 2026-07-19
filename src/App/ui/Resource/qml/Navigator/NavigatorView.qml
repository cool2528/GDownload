import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
import QtQuick.Layouts

Item {
    id: navigator
    Rectangle {
        id: systemNavigator
        objectName: "primaryNavigationRail"
        anchors.fill: parent
        // 跟随主题的导航栏背景:去掉硬编码深色渐变(浅色主题错位 bug 根源)
        color: GTheme.surfaceBase

        // 右侧分隔线,增强分区感
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: GTheme.borderLight
        }

        implicitWidth: GTheme.navBarWidth
        SplitView.minimumWidth: GTheme.navBarWidth

        // 顶部导航
        ColumnLayout {
            id: topLayout
            width: GTheme.navBarWidth
            height: implicitHeight
            anchors.top: parent.top
            anchors.topMargin: GTheme.spaceXL
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: GTheme.spaceMD

            Rectangle {
                id: brandLogo
                objectName: "repositoryLink"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: GTheme.radiusMedium
                color: GTheme.fillLight
                border.width: 1
                border.color: GTheme.borderLighter
                activeFocusOnTab: visible
                Accessible.role: Accessible.Link
                Accessible.name: qsTr("Open GDownload repository")
                Accessible.description: qsTr("Opens in the default browser")

                function activate() {
                    Qt.openUrlExternally("https://github.com/cool2528/GDownload")
                }

                AuroraBrand {
                    anchors.centerIn: parent
                    markSize: 30
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: parent.radius + 3
                    color: "transparent"
                    border.width: 2
                    border.color: GTheme.focusRing
                    visible: brandLogo.activeFocus
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: brandLogo.forceActiveFocus()
                    onClicked: brandLogo.activate()
                }

                Keys.onReturnPressed: event => {
                    brandLogo.activate()
                    event.accepted = true
                }
                Keys.onEnterPressed: event => {
                    brandLogo.activate()
                    event.accepted = true
                }
                Keys.onSpacePressed: event => {
                    brandLogo.activate()
                    event.accepted = true
                }
            }

            GButton {
                id: home
                objectName: "navHome"
                Accessible.name: qsTr("Home")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Home")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                checked: (typeof brower_view !== "undefined") && brower_view.index === 2
                iconName: "home"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    brower_view.index = 2
                }
            }
            GButton {
                id: download
                objectName: "navDownloading"
                Accessible.name: qsTr("Active downloads")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Active downloads")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                checked: (typeof brower_view !== "undefined") && brower_view.index === 0 && brower_view.downloadIndex === 0
                iconName: "download"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    brower_view.index = 0
                    brower_view.switchDownloadPage(0)
                }
            }

            GButton {
                id: waiting
                objectName: "navWaiting"
                Accessible.name: qsTr("Waiting downloads")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Waiting downloads")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                checked: (typeof brower_view !== "undefined") && brower_view.index === 0 && brower_view.downloadIndex === 1
                iconName: "queue"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    brower_view.index = 0
                    brower_view.switchDownloadPage(1)
                }
            }

            GButton {
                id: completed
                objectName: "navCompleted"
                Accessible.name: qsTr("Stopped downloads")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Stopped downloads")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                checked: (typeof brower_view !== "undefined") && brower_view.index === 0 && brower_view.downloadIndex === 2
                iconName: "completed"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    brower_view.index = 0
                    brower_view.switchDownloadPage(2)
                }
            }

            GButton {
                id: addTask
                objectName: "navAddTask"
                Accessible.name: qsTr("Add download")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Add download")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                iconName: "add"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    let task = addDownloadTask()
                    if (task) {
                        task.open()
                    }
                }
            }
        }

        // 底部导航
        ColumnLayout {
            id: bottomLayout
            width: GTheme.navBarWidth
            height: implicitHeight
            anchors.bottom: parent.bottom
            anchors.bottomMargin: GTheme.space3XL
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: GTheme.space2XL

            GButton {
                id: setting
                objectName: "navSettings"
                Accessible.name: qsTr("Preferences")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Preferences")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                checked: (typeof brower_view !== "undefined") && brower_view.index === 1
                iconName: "settings"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    brower_view.index = 1
                }
            }
            GButton {
                id: help
                objectName: "navHelp"
                Accessible.name: qsTr("Help and about")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Help and about")
                Layout.alignment: Qt.AlignHCenter
                radius: GTheme.radiusLarge
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                iconName: "help"
                imageSize: Qt.size(20, 20)
                tintColor: contentColor
                onClicked: {
                    let about = showAboutDialog()
                    if (about) {
                        about.open()
                    }
                }
            }
        }
    }

    // 复用 Component（编译一次），避免每次点击都新建 Component 造成内存泄漏
    Component { id: taskDialogComponent; TaskDialogPage {} }
    Component { id: aboutDialogComponent; HelpDialog {} }

    function addDownloadTask(initialUrl){
        let props = {}
        if (initialUrl && initialUrl.length > 0)
            props.initialUrl = initialUrl
        let task = taskDialogComponent.createObject(mainWindow, props)
        if(task === null){
            console.error("Error creating object")
            return null
        }
        // Popup 的 close() 只置 visible=false 不会销毁对象，需在关闭后手动销毁，
        // 否则每次打开“添加任务”都会在 mainWindow 子树上累积一整棵组件树导致内存泄漏。
        task.closed.connect(function(){ task.destroy() })
        return task
    }

    function showAboutDialog(){
        let about = aboutDialogComponent.createObject(mainWindow)
        if(about === null){
            console.error("Error creating object")
            return null
        }
        about.closed.connect(function(){ about.destroy() })
        return about
    }
}
