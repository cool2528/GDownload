import QtQuick
import org.wangwenx190.FramelessHelper
import QtQuick.Controls
import "Navigator"
import "titlebar"
import "Browser"
FramelessWindow{
    id: mainWindow
    width: 1024
    height: 768
    title: qsTr("GDownload title")
    visible: false
    property var helper: FramelessHelper
    FramelessHelper.onReady: {
        FramelessHelper.titleBarItem = title_bar;
        FramelessHelper.moveWindowToDesktopCenter()
        mainWindow.visible = true;
        if(Qt.platform.os === "osx"){
            UtilsToolsManager.HideMacOsxWindowStandardButtons(mainWindow)
        }
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


}



