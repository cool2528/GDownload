import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

Rectangle {
    id: titleBar
    objectName: "applicationTitleBar"
    property string sectionTitle: qsTr("Home")
    property bool windowActive: true
    width: parent.width
    visible: Qt.platform.os === "osx" ? (mainWindow.fullScreen ? false : true) : true
    color: windowActive ? GTheme.surfaceElevated : GTheme.surfaceBase
    height: visible ? GTheme.titleBarHeight : 0

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: GTheme.borderLighter
    }

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

    RowLayout {
        id: applicationIdentity
        anchors.left: parent.left
        anchors.leftMargin: Qt.platform.os === "osx"
                            ? GTheme.spaceMD + GTheme.sizeDefault * 3
                            : GTheme.spaceMD
        anchors.verticalCenter: parent.verticalCenter
        spacing: GTheme.spaceSM

        AuroraBrand {
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            markSize: 24
        }

        Text {
            text: "GDownload"
            color: GTheme.textPrimary
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightDemiBold
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 16
            color: GTheme.borderLight
        }

        Text {
            text: titleBar.sectionTitle
            color: GTheme.textSecondary
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightRegular
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.maximumWidth: 280
        }
    }

    RowLayout {
        id: macosTitleBar
        spacing: 0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: GTheme.spaceXS
        visible: Qt.platform.os === "osx"

        GImageButton {
            id: close
            Accessible.name: qsTr("Close window")
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: GTheme.sizeDefault
            imageSize: Qt.size(15, 15)
            backgroundColor: "transparent"
            normalImage: "/images/titlebar/macos-close.svg"
            hoverImage: "/images/titlebar/macos-clos-hover.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Close")
            onClicked: {
                titleBar.handleClose()
            }
        }

        GImageButton {
            id: minsize
            Accessible.name: qsTr("Minimize window")
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: GTheme.sizeDefault
            imageSize: Qt.size(15, 15)
            backgroundColor: "transparent"
            normalImage: "/images/titlebar/macos-minimize.svg"
            hoverImage: "/images/titlebar/macos-minimize-hover.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Minimize")
            onClicked: {
                mainWindow.showMinimized2()
            }
        }

        GImageButton {
            id: maxsize
            Accessible.name: qsTr("Enter full screen")
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: GTheme.sizeDefault
            imageSize: Qt.size(15, 15)
            backgroundColor: "transparent"
            normalImage: "/images/titlebar/macos-maximize.svg"
            hoverImage: "/images/titlebar/macos-maximize-hover.svg"
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Full screen")
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
        GImageButton{
            id:win_minsize
            Accessible.name: qsTr("Minimize")
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : Qt.alpha(GTheme.fillLight, 0)
            normalImage: GTheme.dark ? "/images/titlebar/windows-minimize-dark.svg" : "/images/titlebar/windows-minimize.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-minimize-dark.svg" : "/images/titlebar/windows-minimize-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.showMinimized2()
            }
        }
        GImageButton{
            id:win_maxsize
            Accessible.name: qsTr("Maximize")
            visible: !mainWindow.maximized
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : Qt.alpha(GTheme.fillLight, 0)
            normalImage: GTheme.dark ? "/images/titlebar/windows-maximize-dark.svg" : "/images/titlebar/windows-maximize.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-maximize-dark.svg" : "/images/titlebar/windows-maximize-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        //windows-restore

        GImageButton{
            id:win_restore
             Accessible.name: qsTr("Restore")
             visible: mainWindow.maximized
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.fillLight : Qt.alpha(GTheme.fillLight, 0)
            normalImage: GTheme.dark ? "/images/titlebar/windows-restore-dark.svg" : "/images/titlebar/windows-restore.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-restore-dark.svg" : "/images/titlebar/windows-restore-hover.svg"
            tintColor: "transparent"
            onClicked: {
                mainWindow.toggleMaximized()
            }
        }
        GImageButton{
            id:win_close
            Accessible.name: qsTr("Close")
            Layout.preferredHeight: GTheme.titleBarHeight
            Layout.preferredWidth: 40
            imageSize: Qt.size(16,16)
            backgroundColor: hovered ? GTheme.dangerColor : Qt.alpha(GTheme.dangerColor, 0)
            normalImage: GTheme.dark ? "/images/titlebar/windows-close-dark.svg" : "/images/titlebar/windows-close.svg"
            hoverImage: GTheme.dark ? "/images/titlebar/windows-close-dark.svg" : "/images/titlebar/windows-close-hover.svg"
            tintColor: "transparent"
            onClicked: {
                titleBar.handleClose()
            }
        }



    }

}
