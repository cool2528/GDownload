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

    // 系统托盘
    SystemTrayIcon{
        id:systemTray
        icon.mask: true
        icon.source: "qrc:/images/logo/icon.ico"
        visible: true
        tooltip:"GDownload"
        onActivated: function(reason){
            if (reason === SystemTrayIcon.DoubleClick) {
                mainWindow.showNormal()
            }else if(reason === SystemTrayIcon.MiddleClick){
                mainWindow.hide()
            }else if(reason === SystemTrayIcon.Context || reason === SystemTrayIcon.SystemTrayIcon.Trigger){
                tray_menu.open()
            }
        }
        menu: Menu{
            id:tray_menu
            MenuItem{
                text:qsTr("Show main interface")
                onTriggered: {
                    mainWindow.showNormal()
                }
            }
            MenuSeparator{}
            MenuItem{
                text:qsTr("Hide main interface")
                onTriggered: {
                    mainWindow.hide()
                }
            }
            MenuSeparator{}
            MenuItem{
                text:qsTr("Exit")
                onTriggered: {
                   Qt.quit()
                }
            }
        }
    }

}



