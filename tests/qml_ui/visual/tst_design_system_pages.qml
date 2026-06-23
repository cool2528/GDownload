import QtQuick
import QtTest
import "qrc:/tests/qml_ui/support"

TestCase {
    id: testCase
    name: "tst_design_system_pages"
    when: windowShown
    width: 1280
    height: 760

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    property var currentPopup: null

    function init() {
        cleanupPopup()
        harness.themeMode = "light"
        harness.load("")
        wait(50)
    }

    function cleanupPopup() {
        if (currentPopup !== null) {
            try { currentPopup.close() } catch (e) {}
            try { currentPopup.destroy() } catch (e) {}
            currentPopup = null
        }
    }

    function openPopupAndCapture(url, tag, theme, width, height) {
        testCase.width = width
        testCase.height = height
        harness.themeMode = theme
        wait(150)
        var comp = Qt.createComponent(url)
        verify(comp.status === Component.Ready, "Component not ready: " + comp.errorString())
        var popup = comp.createObject(harness)
        verify(popup !== null, "Failed to create popup " + url)
        currentPopup = popup
        popup.parent = harness
        popup.open()
        wait(350)
        var ok = Screenshot.captureWindow(harness, tag, harness.themeMode)
        verify(ok, "Screenshot.captureWindow failed for " + tag)
        cleanupPopup()
    }

    function test_taskdialog_light() {
        openPopupAndCapture("qrc:/qml/CommonComponents/TaskDialogPage.qml", "taskdialog_light", "light", 1280, 760)
    }

    function test_taskdialog_dark() {
        openPopupAndCapture("qrc:/qml/CommonComponents/TaskDialogPage.qml", "taskdialog_dark", "dark", 1280, 760)
    }

    function loadPageAndCapture(url, tag, theme, width, height) {
        testCase.width = width
        testCase.height = height
        harness.themeMode = theme
        harness.load(url)
        wait(300)
        var ok = Screenshot.capture(harness, tag, harness.themeMode)
        verify(ok, "Screenshot.capture failed for " + tag)
    }

    function test_netdisk_workflow_light() {
        loadPageAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_workflow_light", "light", 1180, 720)
    }

    function test_netdisk_workflow_dark() {
        loadPageAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_workflow_dark", "dark", 1180, 720)
    }

    function test_download_empty_light() {
        loadPageAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "download_empty_light", "light", 1280, 760)
    }

    function test_download_empty_dark() {
        loadPageAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "download_empty_dark", "dark", 1280, 760)
    }

    function test_feedback_close_light() {
        openPopupAndCapture("qrc:/qml/CommonComponents/CloseConfirmDialog.qml", "feedback_close_light", "light", 900, 620)
    }

    function test_feedback_close_dark() {
        openPopupAndCapture("qrc:/qml/CommonComponents/CloseConfirmDialog.qml", "feedback_close_dark", "dark", 900, 620)
    }

    function test_feedback_help_light() {
        openPopupAndCapture("qrc:/qml/CommonComponents/HelpDialog.qml", "feedback_help_light", "light", 900, 720)
    }

    function test_feedback_help_dark() {
        openPopupAndCapture("qrc:/qml/CommonComponents/HelpDialog.qml", "feedback_help_dark", "dark", 900, 720)
    }
}
