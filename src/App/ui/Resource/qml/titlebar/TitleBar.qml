import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

Rectangle {
    id: titleBar
    width: parent.width
    color: GTheme.bgWhite
    height: 32
    visible: Qt.platform.os === "osx" ? (mainWindow.fullScreen ? false : true) : true

    // 关闭确认对话框
    CloseConfirmDialog {
        id: closeConfirmDialog

        onActionSelected: function(action, dontAskAgain) {
            // 如果选择了"不再询问"，保存设置
            if (dontAskAgain) {
                SettingsManager.qShowCloseConfirm = false
                if (action === CloseConfirmDialog.MinimizeToTray) {
                    SettingsManager.qCloseToTray = true
                } else if (action === CloseConfirmDialog.Quit) {
                    SettingsManager.qCloseToTray = false
                }
            }

            // 执行对应的操作
            if (action === CloseConfirmDialog.Quit) {
                Qt.quit()
            } else if (action === CloseConfirmDialog.MinimizeToTray) {
                mainWindow.hide()
            }
            // Cancel 不做任何操作
        }
    }

    // 关闭窗口的处理函数
    function handleClose() {
        // 如果设置了不再询问
        if (!SettingsManager.qShowCloseConfirm) {
            if (SettingsManager.qCloseToTray) {
                // 最小化到托盘
                mainWindow.hide()
            } else {
                // 直接退出
                Qt.quit()
            }
        } else {
            // 显示确认对话框
            closeConfirmDialog.open()
        }
    }

    Component.onCompleted: {
       if(Qt.platform.os !== "osx"){
           helper.setHitTestVisible(win_close)
           helper.setHitTestVisible(win_minsize)
           helper.setHitTestVisible(win_maxsize)
           helper.setHitTestVisible(win_restore)
       }
    }

    RowLayout{
        id:macosTitleBar
        spacing: 5
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 10
        visible: Qt.platform.os === "osx"
        ImageButton{
            id:close
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-close.svg"
            hoverImage: "/images/titlebar/macos-clos-hover.svg"
            onClicked: {
                titleBar.handleClose()
            }
        }

        ImageButton{
            id:minsize
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-minimize.svg"
            hoverImage: "/images/titlebar/macos-minimize-hover.svg"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }

        ImageButton{
            id:maxsize
            Layout.preferredHeight: 15
            Layout.preferredWidth: 15
            backgroundColor:"transparent"
            normalImage: "/images/titlebar/macos-maximize.svg"
            hoverImage: "/images/titlebar/macos-maximize-hover.svg"
            onClicked: {
                mainWindow.toggleFullScreen()
            }
        }
    }

    // windows title bar
    RowLayout{
        id:winTitleBar
        spacing: 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: Qt.platform.os !== "osx"
        ImageButton{
            id:win_minsize
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : "transparent"
            normalImage: GTheme.dark ? "/images/titlebar/windows-minimize-dark.svg" : "/images/titlebar/windows-minimize.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-minimize-dark.svg" : "/images/titlebar/windows-minimize-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }
        ImageButton{
            id:win_maxsize
            visible: !mainWindow.maximized
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : "transparent"
            normalImage: GTheme.dark ? "/images/titlebar/windows-maximize-dark.svg" : "/images/titlebar/windows-maximize.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-maximize-dark.svg" : "/images/titlebar/windows-maximize-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        //windows-restore

        ImageButton{
            id:win_restore
             visible: mainWindow.maximized
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : "transparent"
            normalImage: GTheme.dark ? "/images/titlebar/windows-restore-dark.svg" : "/images/titlebar/windows-restore.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-restore-dark.svg" : "/images/titlebar/windows-restore-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        ImageButton{
            id:win_close
            Layout.preferredHeight: 32
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.dangerLight(3) : "transparent"
            normalImage: GTheme.dark ? "/images/titlebar/windows-close-dark.svg" : "/images/titlebar/windows-close.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-close-dark.svg" : "/images/titlebar/windows-close-hover.svg"
            tintColor: "transparent"
            onClicked: {
                titleBar.handleClose()
            }
        }



    }

}
