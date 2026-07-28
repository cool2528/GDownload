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
    objectName: "mainWindow"
    width: 1024
    height: 768
    minimumWidth: 900
    minimumHeight: 640
    color: GTheme.bgPage
    title:"GDownload"
    visible: false
    property var helper: FramelessHelper
    FramelessHelper.onReady: {
        FramelessHelper.titleBarItem = title_bar;
        FramelessHelper.moveWindowToDesktopCenter()
        restoreSavedGeometry()
        // --silent 静默启动：不显示主窗口，仅驻留系统托盘（供 host launch 静默唤起）
        if (typeof gAppStartSilent === "undefined" || !gAppStartSilent) {
            mainWindow.visible = true;
        }
        if(Qt.platform.os === "osx"){
            UtilsToolsManager.HideMacOsxWindowStandardButtons(mainWindow)
        }
    }

    // 恢复上次的窗口几何。调用点在 moveWindowToDesktopCenter() 之后：凡是这里决定“不恢复”的
    // 情形，都会保留居中的结果。
    function restoreSavedGeometry(){
        if(!SettingsManager.qRememberWindowPosition) return

        var sz = SettingsManager.qWindowSize
        // 尺寸下限保护：保存成 (0,0) 会把窗口设为不可见。
        if(sz.width <= 100 || sz.height <= 100) return
        mainWindow.width = sz.width
        mainWindow.height = sz.height

        var pos = SettingsManager.qWindowPosition
        // 【位置从没保存过时保持居中】首次启动配置里 general.window-position 是空串
        // (config_key.h 的默认值)，解析不出 "x,y" 两段就退回 Setting 的 Default()
        // = QPoint(0,0)(setting.h 的 WindowPosition)。旧逻辑只用尺寸判“是否已初始化”，
        // 于是这个 (0,0) 被当成“保存过的位置”写回去，把上面 moveWindowToDesktopCenter()
        // 的结果覆盖掉 —— 首次启动窗口贴在桌面左上角就是这么来的。
        // 代价：用户特意把窗口摆在正好 (0,0) 时，下次启动会变成居中。这是可接受的退化，
        // 而且比“新装用户第一次打开就贴边”好得多。
        if(pos.x === 0 && pos.y === 0) return

        // 保存的位置可能来自已经拔掉的显示器，直接用会让窗口落在所有屏幕之外、无法操作。
        // 只要标题栏那一条还有一部分落在某块屏幕上就认为可用。
        if(!geometryVisibleOnSomeScreen(pos, sz)) return

        mainWindow.x = pos.x
        mainWindow.y = pos.y
    }

    // 判断给定几何是否还有可抓取的部分留在某块屏幕内(以标题栏高度为准)。
    function geometryVisibleOnSomeScreen(pos, sz){
        var screens = Qt.application.screens
        if(!screens || screens.length === 0) return true   // 取不到屏幕信息时不阻拦恢复
        var grabH = Math.max(24, title_bar ? title_bar.height : 32)
        for(var i = 0; i < screens.length; ++i){
            var s = screens[i]
            var ix = Math.max(pos.x, s.virtualX)
            var iy = Math.max(pos.y, s.virtualY)
            var ax = Math.min(pos.x + sz.width,  s.virtualX + s.width)
            var ay = Math.min(pos.y + grabH,     s.virtualY + s.height)
            if(ax - ix > 80 && ay - iy > 8) return true
        }
        return false
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
        sectionTitle: brower_view.currentSectionTitle
        windowActive: mainWindow.active
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
        icon.mask: Qt.platform.os === "osx"  // 仅 macOS 启用 mask 自动适配主题;Windows/Linux 保留彩色图标(Q4)
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

    // 单实例：后续实例启动时提升已有主窗口
    Connections {
        target: BrowserManager
        function onSigActivateWindow() {
            mainWindow.showNormal()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
    }

}



