import QtQuick
import QtTest
import gdl.sdk
import "qrc:/tests/qml_ui/support"

// 视觉用例:5 个对话框 × 2 主题 = 10 张截图(Task 8)
//
// 覆盖范围:
//   - CloseConfirmDialog  关闭确认(GMessageBox/Dialog 子类)
//   - HelpDialog          关于(GDialogShell/Popup 子类)
//   - UpdateDialog        更新(GDialogShell/Popup 子类)
//   - DeleteConfirmDialog 删除确认(GMessageBox/Dialog 子类)
//   - GMessageBox(4-type) 通用消息框 Info/Success/Warning/Error 横向并排
//
// === 渲染策略(分两类,因 Dialog 与 Popup 在 offscreen 平台行为差异)===
//
// 1. GDialogShell(Popup)类:HelpDialog、UpdateDialog
//    Popup.open() 后内容渲染到窗口 Overlay(QQuickOverlay,是 QQuickWindow
//    contentItem 的子项)。Screenshot.captureWindow(harness, ...) 抓取窗口
//    contentItem,可一并捕获 Overlay 上的 Popup 全貌(背景+表头+内容+footer)。
//
// 2. GMessageBox(Dialog)类:CloseConfirmDialog、DeleteConfirmDialog、GMessageBox
//    Dialog 的 popupItem 虽也在 Overlay 上,但 captureWindow 抓 contentItem 时
//    Dialog 的 popupItem 内容不被捕获(疑似 Qt offscreen 平台对 Dialog 的
//    padding/header 分区渲染与 grabToImage 的交互问题)。
//    回退方案:open() 后将 dlg.contentItem reparent 到 harness(脱离 Overlay),
//    再用 Screenshot.capture(contentItem, ...) 直接抓取。contentItem 包含
//    消息文本 + customContent + 按钮区域(无 header/background 装饰),
//    虽缺少对话框外壳,但保留了核心内容区域的视觉回归基线。
//    Task 9 重构主项目为库后,可在真实窗口环境中补充 Dialog 全貌截图。
TestCase {
    id: testCase
    name: "tst_dialogs"
    when: windowShown
    width: 800
    height: 600

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    SignalSpy {
        id: messageBoxSpy
        signalName: "buttonClicked"
    }

    SignalSpy {
        id: closeActionSpy
        signalName: "actionSelected"
    }

    SignalSpy {
        id: deleteActionSpy
        signalName: "actionSelected"
    }

    // 4 个单对话框(各含 qrc 路径 + 窗口尺寸 + tag 后缀 + 是否为 Dialog 类)
    property var dialogs: [
        { tag: "closeconfirm",  path: "qrc:/qml/CommonComponents/CloseConfirmDialog.qml",  w: 480, h: 440, isDialog: true },
        { tag: "help",          path: "qrc:/qml/CommonComponents/HelpDialog.qml",          w: 700, h: 580, isDialog: false },
        { tag: "update",        path: "qrc:/qml/CommonComponents/UpdateDialog.qml",        w: 620, h: 660, isDialog: false },
        { tag: "deleteconfirm", path: "qrc:/qml/CommonComponents/DeleteConfirmDialog.qml", w: 480, h: 380, isDialog: true }
    ]
    property var themes: ["light", "dark"]

    // 当前打开的对话框引用(cleanup 时关闭销毁,避免泄漏)
    property var currentDialog: null
    property var currentCaptureHost: null

    Component {
        id: dialogCaptureHostComponent

        Rectangle {
            id: captureHost
            property alias surface: dialogSurface
            property int surfaceWidth: 460
            property int surfaceHeight: 330

            width: 1024
            height: 768
            color: GTheme.bgPage

            Rectangle {
                anchors.fill: parent
                color: GTheme.overlayScrim
            }

            Rectangle {
                id: dialogSurface
                width: captureHost.surfaceWidth
                height: captureHost.surfaceHeight
                anchors.centerIn: parent
                color: GTheme.surfaceElevated
                radius: GTheme.radiusLarge
                border.width: 1
                border.color: GTheme.borderBase
            }
        }
    }

    function init() {
        harness.themeMode = "light"
        testCase.width = 800
        testCase.height = 600
        cleanupDialog()
        harness.load("")
        wait(20)
    }

    function cleanupDialog() {
        if (testCase.currentDialog !== null) {
            try {
                if (testCase.currentDialog.visible) {
                    testCase.currentDialog.close()
                }
            } catch (e) {}
            try { testCase.currentDialog.destroy() } catch (e) {}
            testCase.currentDialog = null
        }
        if (testCase.currentCaptureHost !== null) {
            try { testCase.currentCaptureHost.destroy() } catch (e) {}
            testCase.currentCaptureHost = null
        }
    }

    function capturePopupContent(popup, tag, theme, separateHeader) {
        var host = dialogCaptureHostComponent.createObject(harness, {
            surfaceWidth: popup.width,
            surfaceHeight: popup.height
        })
        verify(host !== null)
        testCase.currentCaptureHost = host
        host.x = Math.round((harness.width - host.width) / 2)
        host.y = Math.round((harness.height - host.height) / 2)

        var headerHeight = 0
        if (separateHeader && popup.header !== null) {
            headerHeight = popup.header.height > 0 ? popup.header.height : popup.header.implicitHeight
            popup.header.parent = host.surface
            popup.header.x = 0
            popup.header.y = 0
            popup.header.width = host.surface.width
            popup.header.height = headerHeight
        }
        popup.contentItem.parent = host.surface
        popup.contentItem.x = 0
        popup.contentItem.y = headerHeight
        popup.contentItem.width = host.surface.width
        popup.contentItem.height = host.surface.height - headerHeight
        wait(150)
        verify(Screenshot.capture(host, tag, theme), "Screenshot capture failed for " + tag)
    }

    // 通用:createComponent + createObject → open() → capture
    // Dialog 类:reparent contentItem 到 harness 后直接抓(绕过 Overlay 限制)
    // Popup 类:captureWindow 抓窗口 contentItem(含 Overlay)
    function createOpenAndCapture(url, tag, theme, winW, winH, isDialog, properties) {
        harness.themeMode = theme
        testCase.width = winW
        testCase.height = winH
        // 等窗口尺寸应用 + 场景图重排
        wait(150)

        var comp = Qt.createComponent(url)
        verify(comp.status === Component.Ready, "Component not ready for " + url + ": " + comp.errorString())

        // createObject(harness) 设置 Qt 父对象;dlg.parent = harness 设置 Popup 可视 parent
        var dlg = comp.createObject(harness)
        verify(dlg !== null, "Failed to create dialog object from " + url)
        testCase.currentDialog = dlg
        dlg.parent = harness
        if (properties !== undefined && properties !== null) {
            for (var propertyName in properties) {
                dlg[propertyName] = properties[propertyName]
            }
        }
        // 关闭 modal:dim 层可能干扰 grabToImage 对 contentItem 的捕获
        try { dlg.modal = false } catch (e) {}

        if (typeof dlg.open === "function") {
            dlg.open()
            // enter 动画 durationBase=150ms,留余量到 300ms
            wait(300)
        }

        verify(dlg.x >= -0.5, tag + " has a negative x coordinate")
        verify(dlg.y >= -0.5, tag + " has a negative y coordinate")
        verify(dlg.x + dlg.width <= winW + 0.5,
               tag + " extends beyond the right edge")
        verify(dlg.y + dlg.height <= winH + 0.5,
               tag + " extends beyond the bottom edge")

        var ok
        if (isDialog) {
            // Dialog 类:reparent contentItem 到 harness 使其可被 grabToImage 捕获
            // contentItem 包含消息文本 + customContent + 按钮区域
            verify(dlg.contentItem !== null, "Dialog contentItem is null for " + tag)
            dlg.contentItem.parent = harness
            dlg.contentItem.x = Math.max(0, (winW - dlg.contentItem.width) / 2)
            dlg.contentItem.y = Math.max(0, (winH - dlg.contentItem.height) / 2)
            // 等 reparent 后布局重排 + 场景图渲染
            wait(150)
            ok = Screenshot.capture(dlg.contentItem, tag, harness.themeMode)
        } else {
            // Popup 类:captureWindow 抓窗口 contentItem(含 Overlay 层)
            ok = Screenshot.captureWindow(harness, tag, harness.themeMode)
        }
        verify(ok, "Screenshot capture failed for " + tag)

        cleanupDialog()
    }

    // 数据驱动:4 对话框 × 2 主题 = 8 张截图
    function test_all_dialogs() {
        for (var i = 0; i < dialogs.length; ++i) {
            for (var t = 0; t < themes.length; ++t) {
                var tag = dialogs[i].tag + "_" + themes[t]
                createOpenAndCapture(dialogs[i].path, tag, themes[t], dialogs[i].w, dialogs[i].h, dialogs[i].isDialog)
            }
        }
    }

    function test_narrow_dialogs_light_dark() {
        var narrowDialogs = [
            {
                tag: "closeconfirm_narrow",
                path: "qrc:/qml/CommonComponents/CloseConfirmDialog.qml",
                isDialog: true,
                properties: null
            },
            {
                tag: "deleteconfirm_narrow",
                path: "qrc:/qml/CommonComponents/DeleteConfirmDialog.qml",
                isDialog: true,
                properties: {
                    pageType: 2,
                    taskFileName: "A very long multilingual-ready download task name that must remain inside the confirmation surface.zip"
                }
            },
            {
                tag: "update_narrow",
                path: "qrc:/qml/CommonComponents/UpdateDialog.qml",
                isDialog: false,
                properties: {
                    versionNumber: "v9.99.0-preview",
                    releaseNotes: "## Highlights\n\n- A deliberately long release note validates wrapping and scrolling in a narrow window.\n- Download recovery and update verification are improved."
                }
            },
            {
                tag: "help_narrow",
                path: "qrc:/qml/CommonComponents/HelpDialog.qml",
                isDialog: false,
                properties: null
            }
        ]

        for (var i = 0; i < narrowDialogs.length; ++i) {
            for (var t = 0; t < themes.length; ++t) {
                var config = narrowDialogs[i]
                createOpenAndCapture(config.path,
                                     config.tag + "_" + themes[t],
                                     themes[t], 360, 520,
                                     config.isDialog, config.properties)
            }
        }
    }

    function captureUpdateState(theme, state, tag) {
        harness.themeMode = theme
        testCase.width = 620
        testCase.height = 660
        wait(150)

        var comp = Qt.createComponent("qrc:/qml/CommonComponents/UpdateDialog.qml")
        verify(comp.status === Component.Ready, comp.errorString())
        var dlg = comp.createObject(harness, {
            versionNumber: "v3.2.1",
            releaseNotes: "## Aurora update\n\nThis release improves task recovery, accessibility, and visual consistency."
        })
        verify(dlg !== null, "Failed to create UpdateDialog")
        testCase.currentDialog = dlg
        dlg.parent = harness
        dlg.modal = false
        dlg.open()
        wait(300)

        if (state === "progress") {
            dlg.handleUpdateProgress({ stage: 1, percentage: 58,
                                       message: "Downloading update package..." })
        } else if (state === "failed") {
            dlg.updateState = "failed"
            dlg.updating = false
            dlg.failureMessage = "The package could not be verified. Check your connection and try again."
        }
        wait(150)

        compare(dlg.updateState, state)
        verify(dlg.x >= 0 && dlg.y >= 0)
        verify(dlg.x + dlg.width <= testCase.width + 0.5)
        verify(dlg.y + dlg.height <= testCase.height + 0.5)
        capturePopupContent(dlg, tag, harness.themeMode, false)
        cleanupDialog()
    }

    function test_update_progress_dark() {
        captureUpdateState("dark", "progress", "update_progress_dark")
    }

    function test_update_failed_light() {
        captureUpdateState("light", "failed", "update_failed_light")
    }

    function test_delete_full_window_light() {
        testCase.width = 1024
        testCase.height = 768
        harness.themeMode = "light"
        wait(120)

        var comp = Qt.createComponent("qrc:/qml/CommonComponents/DeleteConfirmDialog.qml")
        verify(comp.status === Component.Ready, comp.errorString())
        var dlg = comp.createObject(harness, {
            pageType: 2,
            taskFileName: "creative-assets-2026.zip"
        })
        verify(dlg !== null)
        testCase.currentDialog = dlg
        dlg.parent = harness
        dlg.open()
        wait(350)
        capturePopupContent(dlg, "deleteconfirm_full_light", "light", true)
        cleanupDialog()
    }

    function test_update_primary_focus_enter_and_escape() {
        testCase.width = 620
        testCase.height = 660
        wait(100)

        var comp = Qt.createComponent("qrc:/qml/CommonComponents/UpdateDialog.qml")
        verify(comp.status === Component.Ready, comp.errorString())
        var dlg = comp.createObject(harness, {
            versionNumber: "v3.2.1",
            releaseNotes: "Keyboard behavior regression coverage."
        })
        verify(dlg !== null)
        testCase.currentDialog = dlg
        dlg.parent = harness
        dlg.modal = false
        dlg.open()
        wait(250)

        verify(dlg.primaryActionButton !== null, "Update primary action is unavailable")
        verify(dlg.primaryActionButton.activeFocus,
               "Update primary action should receive initial focus")
        keyClick(Qt.Key_Return)
        tryCompare(dlg, "updateState", "failed")
        verify(dlg.visible, "A failed update start should keep the dialog open")

        keyClick(Qt.Key_Escape)
        tryVerify(function() { return !dlg.visible })
        cleanupDialog()
    }

    function test_messagebox_keyboard_and_narrow_geometry() {
        testCase.width = 340
        testCase.height = 440
        wait(100)

        var comp = Qt.createComponent("qrc:/qml/CommonComponents/GMessageBox.qml")
        verify(comp.status === Component.Ready, comp.errorString())
        var box = comp.createObject(harness, {
            title: "A confirmation title that must stay bounded",
            message: "This deliberately long message validates word wrapping, scrolling, and keyboard activation without allowing text or actions to escape the narrow dialog surface.",
            defaultButtonIndex: 1,
            buttons: [
                { text: "Cancel", type: "default", width: 96 },
                { text: "Continue", type: "primary", width: 112 },
                { text: "More Options", type: "default", width: 120 }
            ]
        })
        verify(box !== null, "Failed to create narrow GMessageBox")
        testCase.currentDialog = box
        box.parent = harness
        box.modal = false
        messageBoxSpy.target = box
        messageBoxSpy.clear()
        box.open()
        wait(250)

        verify(box.stackButtons, "Narrow message box actions should stack")
        verify(box.x >= 0 && box.y >= 0)
        verify(box.x + box.width <= testCase.width + 0.5)
        verify(box.y + box.height <= testCase.height + 0.5)
        keyClick(Qt.Key_Return)
        tryCompare(messageBoxSpy, "count", 1)
        compare(messageBoxSpy.signalArguments[0][0], 1)
        tryVerify(function() { return !box.visible })

        box.open()
        wait(200)
        keyClick(Qt.Key_Escape)
        tryVerify(function() { return !box.visible })
        messageBoxSpy.target = null
        cleanupDialog()
    }

    function test_delete_and_close_default_keyboard_actions() {
        testCase.width = 620
        testCase.height = 520
        wait(100)

        var closeComponent = Qt.createComponent("qrc:/qml/CommonComponents/CloseConfirmDialog.qml")
        verify(closeComponent.status === Component.Ready, closeComponent.errorString())
        var closeDialog = closeComponent.createObject(harness)
        verify(closeDialog !== null)
        testCase.currentDialog = closeDialog
        closeDialog.parent = harness
        closeDialog.modal = false
        closeActionSpy.target = closeDialog
        closeActionSpy.clear()
        closeDialog.open()
        wait(250)
        keyClick(Qt.Key_Return)
        tryCompare(closeActionSpy, "count", 1)
        compare(closeActionSpy.signalArguments[0][0], 2) // MinimizeToTray
        tryVerify(function() { return !closeDialog.visible })
        closeActionSpy.target = null
        cleanupDialog()

        var deleteComponent = Qt.createComponent("qrc:/qml/CommonComponents/DeleteConfirmDialog.qml")
        verify(deleteComponent.status === Component.Ready, deleteComponent.errorString())
        var deleteDialog = deleteComponent.createObject(harness, {
            pageType: 2,
            taskFileName: "keyboard-delete.zip"
        })
        verify(deleteDialog !== null)
        testCase.currentDialog = deleteDialog
        deleteDialog.parent = harness
        deleteDialog.modal = false
        deleteActionSpy.target = deleteDialog
        deleteActionSpy.clear()
        deleteDialog.open()
        wait(250)
        keyClick(Qt.Key_Return)
        tryCompare(deleteActionSpy, "count", 1)
        compare(deleteActionSpy.signalArguments[0][0], 0) // Cancel
        tryVerify(function() { return !deleteDialog.visible })

        deleteDialog.open()
        wait(200)
        deleteActionSpy.clear()
        keyClick(Qt.Key_Escape)
        tryVerify(function() { return !deleteDialog.visible })
        compare(deleteActionSpy.count, 0)
        deleteActionSpy.target = null
        cleanupDialog()
    }

    // GMessageBox 多类型:4 种 messageType 横向并排,每主题 1 张截图
    // 每个实例 open() 后 reparent contentItem 到容器,抓容器
    function test_gmessagebox_light() {
        gmessageboxCapture("light")
    }

    function test_gmessagebox_dark() {
        gmessageboxCapture("dark")
    }

    function gmessageboxCapture(theme) {
        harness.themeMode = theme
        // 4 × (420 + 24) - 24 + 边距 = ~1800;高度容纳 280 + 上下边距
        testCase.width = 1820
        testCase.height = 380
        wait(150)

        var comp = Qt.createComponent("qrc:/qml/CommonComponents/GMessageBox.qml")
        verify(comp.status === Component.Ready, "GMessageBox component not ready: " + comp.errorString())

        // messageType 枚举:Info=0 / Warning=1 / Error=2 / Success=3 / Question=4
        var configs = [
            { type: 0, title: "Info",    msg: "This is an informational message." },
            { type: 3, title: "Success", msg: "Operation completed successfully." },
            { type: 1, title: "Warning", msg: "This action may have consequences." },
            { type: 2, title: "Error",   msg: "An error occurred during the operation." }
        ]

        var boxes = []
        var boxWidth = 420
        var boxHeight = 280
        var spacing = 24

        for (var i = 0; i < configs.length; ++i) {
            var box = comp.createObject(harness)
            verify(box !== null, "Failed to create GMessageBox instance")
            box.parent = harness
            try { box.modal = false } catch (e) {}
            box.dialogWidth = boxWidth
            box.standardHeight = boxHeight
            box.messageType = configs[i].type
            verify(box.getTypeIconName().length > 0,
                   "GMessageBox semantic icon is undefined for messageType " + configs[i].type)
            box.title = configs[i].title
            box.message = configs[i].msg
            box.buttons = [{ text: "OK", type: "primary", width: 90 }]
            box.defaultButtonIndex = 0
            box.x = i * (boxWidth + spacing)
            box.y = 32
            box.open()
            boxes.push(box)
        }

        // 4 个实例串行创建 + enter 动画,留 500ms 余量
        wait(500)

        // 创建容器,将 4 个 GMessageBox 的 contentItem reparent 进来横向排列
        // reparent 到 harness 下的容器使 contentItem 可被 grabToImage 捕获
        var container = Qt.createQmlObject('import QtQuick; Item { visible: true }', harness)
        container.width = testCase.width
        container.height = testCase.height
        container.x = 0
        container.y = 0

        for (var j = 0; j < boxes.length; ++j) {
            if (boxes[j].contentItem) {
                boxes[j].contentItem.parent = container
                boxes[j].contentItem.x = j * (boxWidth + spacing)
                boxes[j].contentItem.y = 32
            }
        }

        // 等 reparent 后布局重排 + 场景图渲染
        wait(200)

        var tag = "gmessagebox_" + theme
        var ok = Screenshot.capture(container, tag, harness.themeMode)
        verify(ok, "Screenshot.capture failed for " + tag)

        // 清理
        try { container.destroy() } catch (e) {}
        for (var k = 0; k < boxes.length; ++k) {
            try { boxes[k].close() } catch (e) {}
            try { boxes[k].destroy() } catch (e) {}
        }
    }

}
