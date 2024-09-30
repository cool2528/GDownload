import QtQuick
import org.wangwenx190.FramelessHelper
import "./titlebar"
FramelessWindow{
    id: mainWindow
    width: 800
    height: 600
    title: qsTr("GDownload title")
    color: "#242424"
    visible: false // Hide the window before we sets up it's correct size and position.
    FramelessHelper.onReady: {
        FramelessHelper.titleBarItem = titleBar;
        if (Qt.platform.os === "windows") {
            FramelessHelper.setSystemButton(titleBar.minimizeButton, FramelessHelperConstants.Minimize);
            FramelessHelper.setSystemButton(titleBar.maximizeButton, FramelessHelperConstants.Maximize);
            FramelessHelper.setSystemButton(titleBar.closeButton, FramelessHelperConstants.Close);
        }

        FramelessHelper.moveWindowToDesktopCenter()
        mainWindow.visible = true;
        console.log("titleBar ",titleBar.hideWhenClose)
        
    }

    TitleBar{
        id:titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }
}



