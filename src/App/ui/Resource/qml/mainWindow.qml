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
        // 以尺寸有效性作为“已初始化”判据：旧逻辑用 x>0||y>0，
        // 当窗口停在屏幕左上角(0,0)时条件为 false 导致位置/尺寸都不恢复；
        // 同时对尺寸做下限保护，避免保存的 (0,0) 把窗口设为不可见。
        if(SettingsManager.qRememberWindowPosition
           && SettingsManager.qWindowSize.width > 100
           && SettingsManager.qWindowSize.height > 100){
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
    // 拖动/缩放窗口时事件高频触发，用 Timer 防抖，停止移动后再持久化，避免大量 I/O 造成卡顿。
    Timer {
        id: resizeSaveTimer
        interval: 400
        repeat: false
        onTriggered: onWindowResize()
    }
    onWidthChanged: resizeSaveTimer.restart()
    onHeightChanged: resizeSaveTimer.restart()
    onXChanged: resizeSaveTimer.restart()
    onYChanged: resizeSaveTimer.restart()
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
            // 默认 1px 分隔线,悬停/拖动时加宽到 3px 并显示主色提示
            implicitWidth: (SplitHandle.hovered || SplitHandle.pressed) ? 3 : 1
            color: (SplitHandle.hovered || SplitHandle.pressed) ? GTheme.primaryColor : GTheme.borderLight
            Behavior on implicitWidth { NumberAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard } }
            Behavior on color { ColorAnimation { duration: GTheme.durationBase } }
        }
        NavigatorView{
            id:navigator_view
            SplitView.minimumWidth: GTheme.navBarWidth
            SplitView.maximumWidth: GTheme.navBarWidth
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

    UpdateDialog{
        id: updateDialog
    }

    // 系统托盘
    SystemTrayIcon {
        id: systemTray
        icon.mask: Qt.platform.os === "osx" ? true : true  // macOS 启用 mask 以支持自动主题适配
        icon.source: Qt.platform.os === "osx" ? "qrc:/images/logo/tray_template_apple.svg" : "qrc:/images/logo/icon.ico"
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



