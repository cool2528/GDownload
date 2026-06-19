import QtQuick
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
import QtQuick.Layouts

Item {
    id: navigator
    Rectangle {
        id: systemNavigator
        anchors.fill: parent
        // 跟随主题的导航栏背景:去掉硬编码深色渐变(浅色主题错位 bug 根源)
        color: GTheme.bgBase

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
            anchors.top: parent.top
            anchors.topMargin: GTheme.spaceXL
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: GTheme.space2XL

            GButton {
                id: home
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: GTheme.sizeLarge
                implicitHeight: GTheme.sizeLarge
                iconSource: SegoeFluentIcons.HomeSolid
                iconSize: GTheme.fontH1
                onClicked: {
                    Qt.openUrlExternally("https://github.com/cool2528/GDownload")
                }
            }
            GButton {
                id: download
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: GTheme.sizeLarge
                implicitHeight: GTheme.sizeLarge
                iconSource: SegoeFluentIcons.SubscriptionAdd
                iconSize: GTheme.fontH1
                onClicked: {
                    console.debug("select download page")
                    brower_view.index = 0
                }
            }

            GButton {
                id: addTask
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: GTheme.sizeLarge
                implicitHeight: GTheme.sizeLarge
                // 用字体图标(随主题 textSecondary→primary),替代仅适配深色栏的白色 SVG,
                // 避免浅色主题下白色"+"不可见
                iconSource: SegoeFluentIcons.Add
                iconSize: GTheme.fontH1
                onClicked: {
                    console.debug("open add task dialog")
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
            anchors.bottom: parent.bottom
            anchors.bottomMargin: GTheme.space3XL
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: GTheme.space2XL

            GButton {
                id: setting
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: GTheme.sizeLarge
                implicitHeight: GTheme.sizeLarge
                iconSource: SegoeFluentIcons.SettingsSolid
                iconSize: GTheme.fontH1
                onClicked: {
                    console.debug("open settings dialog")
                    brower_view.index = 1
                }
            }
            GButton {
                id: help
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: GTheme.sizeLarge
                implicitHeight: GTheme.sizeLarge
                iconSource: SegoeFluentIcons.Info
                iconSize: GTheme.fontH1
                onClicked: {
                    console.debug("open help dialog")
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

    function addDownloadTask(){
        let task = taskDialogComponent.createObject(mainWindow)
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
