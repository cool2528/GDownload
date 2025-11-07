import QtQuick
import org.wangwenx190.FramelessHelper
import QtQuick.Controls.Basic
import QtQuick.Controls
import gdl.sdk
import Qt.labs.platform
import "Navigator"
import "titlebar"
import "Browser"
import "CommonComponents"
FramelessWindow{
    id: mainWindow
    width: 1024
    height: 768
    title:"GDownload"
    visible: false
    property var helper: FramelessHelper
    FramelessHelper.onReady: {
        FramelessHelper.titleBarItem = title_bar;
        FramelessHelper.moveWindowToDesktopCenter()
        if(SettingsManager.qRememberWindowPosition && (SettingsManager.qWindowPosition.x > 0 || SettingsManager.qWindowPosition.y > 0)){
            mainWindow.x = SettingsManager.qWindowPosition.x
            mainWindow.y = SettingsManager.qWindowPosition.y
            mainWindow.width = SettingsManager.qWindowSize.width
            mainWindow.height = SettingsManager.qWindowSize.height
        }
        mainWindow.visible = true;
        if(Qt.platform.os === "osx"){
            UtilsToolsManager.HideMacOsxWindowStandardButtons(mainWindow)
        }
    }

    function onWindowResize(){
        // 记录窗口当前 x y width height
        var x = mainWindow.x;
        var y = mainWindow.y;
        var width = mainWindow.width;
        var height = mainWindow.height;
        SettingsManager.qWindowPosition = Qt.point(x,y)
        SettingsManager.qWindowSize = Qt.size(width,height)
    }
    onWidthChanged: {
        Qt.callLater(onWindowResize)
    }
    onHeightChanged: {
        Qt.callLater(onWindowResize)
    }
    onXChanged: {
         Qt.callLater(onWindowResize)
    }
    onYChanged: {
        Qt.callLater(onWindowResize)
    }
    onVisibilityChanged: {
        if(Qt.platform.os === "osx"){
            UtilsToolsManager.HideMacOsxWindowStandardButtons(mainWindow)
        }
    }
    TitleBar{
        id:title_bar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }
    SplitView{
        id:main_splitview
        anchors.top: title_bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0

        handle: Rectangle{
            id:handleDelegate
            implicitWidth: 0
            color: "transparent"
        }
        NavigatorView{
            id:navigator_view
            SplitView.minimumWidth: 74
            SplitView.maximumWidth: 74
        }

        BrowserView{
            id:brower_view
            SplitView.fillWidth: true
            SplitView.minimumWidth: 600
            SplitView.preferredWidth: 600
        }

    }

    ToastContainer {
        id: toastContainer
        anchors.fill: parent
        z: 999999  // 确保显示在最上层
    }

    // GMessage 消息容器（增强版）
    GMessageContainer {
        id: messageContainer
        anchors.fill: parent
        z: 1000000  // 比 ToastContainer 更高，确保显示在最上层

        Component.onCompleted: {
            // 连接 MessageManager 信号
            MessageManager.messageRequested.connect(function(options) {
                if (options.action === "close") {
                    messageContainer.closeMessageById(options.id)
                } else if (options.action === "closeAll") {
                    messageContainer.closeAllMessages()
                } else {
                    messageContainer.showMessage(options)
                }
            })
        }
    }

    UpdateDialog{
        id: updateDialog
    }

    // 系统托盘
    SystemTrayIcon {
        id: systemTray
        icon.mask: Qt.platform.os === "osx" ? false : true  // 这个属性会自动处理 macOS 下的 template 模式
        icon.source: Qt.platform.os === "osx" ? "qrc:/images/logo/logo.svg" : "qrc:/images/logo/icon.ico"
        visible: true
        tooltip: "GDownload"

        onActivated: function(reason) {
            if (Qt.platform.os === "osx") {
                // macOS 下点击托盘图标显示菜单
                if (reason === SystemTrayIcon.Trigger) {
                    // 不要直接调用 open，而是使用 Qt.callLater 来避免递归
                    Qt.callLater(function() {
                        tray_menu.open()
                    })
                }
            } else {
                // Windows/Linux 保持原有行为
                if (reason === SystemTrayIcon.DoubleClick) {
                    mainWindow.showNormal()
                    mainWindow.raise()
                    mainWindow.requestActivate()
                } else if (reason === SystemTrayIcon.MiddleClick) {
                    mainWindow.hide()
                } else if (reason === SystemTrayIcon.Context || reason === SystemTrayIcon.Trigger) {
                    Qt.callLater(function() {
                        tray_menu.open()
                    })
                }
            }
        }

        menu: Menu {
            id: tray_menu

            // macOS 专用标题项
            MenuItem {
                text: "GDownload"
                enabled: false
                visible: Qt.platform.os === "osx"
            }
            MenuSeparator {
                visible: Qt.platform.os === "osx"
            }

            MenuItem {
                text: qsTr("Show main interface")
                onTriggered: {
                    if (Qt.platform.os === "osx") {
                        mainWindow.show()
                        mainWindow.raise()
                        mainWindow.requestActivate()
                    } else {
                        mainWindow.showNormal()
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Hide main interface")
                onTriggered: {
                    mainWindow.hide()
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Exit")
                onTriggered: {
                    Qt.quit()
                }
            }
        }
    }

}



