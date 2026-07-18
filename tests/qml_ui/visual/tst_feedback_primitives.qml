import QtQuick
import QtTest
import gdl.sdk
import "qrc:/tests/qml_ui/support"

TestCase {
    id: testCase
    name: "tst_feedback_primitives"
    when: windowShown
    width: 520
    height: 760

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    Rectangle {
        id: stage
        anchors.fill: parent
        z: 1
        color: GTheme.bgPage
    }

    property var toastHost: null
    property var alertItem: null

    function init() {
        cleanup()
        harness.themeMode = "light"
        wait(50)
    }

    function cleanup() {
        if (alertItem !== null) {
            try { alertItem.destroy() } catch (e) {}
            alertItem = null
        }
        if (toastHost !== null) {
            try { toastHost.destroy() } catch (e) {}
            toastHost = null
        }
    }

    function createFeedback(theme) {
        harness.themeMode = theme
        wait(100)

        var toastComponent = Qt.createComponent("qrc:/qml/CommonComponents/ToastContainer.qml")
        verify(toastComponent.status === Component.Ready,
               "ToastContainer component not ready: " + toastComponent.errorString())
        toastHost = toastComponent.createObject(stage, {
            width: stage.width,
            height: 540
        })
        verify(toastHost !== null, "Failed to create ToastContainer")

        var messages = [
            "Download completed and the file is ready to open from the selected destination folder.",
            "A clipboard link was detected. Review the complete address before adding it to the download queue.",
            "Baidu authentication may expire before the next request; open Preferences and refresh the saved cookie.",
            "The connection failed after multiple attempts. Check the network and retry when connectivity is restored."
        ]
        for (var i = 0; i < messages.length; ++i)
            toastHost.showToast(messages[i], i, 0)

        var alertComponent = Qt.createComponent("qrc:/qml/CommonComponents/AlertTip.qml")
        verify(alertComponent.status === Component.Ready,
               "AlertTip component not ready: " + alertComponent.errorString())
        alertItem = alertComponent.createObject(stage, {
            x: 20,
            y: 590,
            width: stage.width - 40,
            severity: "warning",
            title: "Authentication needs attention",
            description: "The saved cloud cookie expires soon. Refresh it before parsing another long-running share request.",
            actionLabel: "Open settings",
            showClose: true
        })
        verify(alertItem !== null, "Failed to create AlertTip")
        wait(450)
    }

    function verifyGeometry() {
        compare(toastHost.activeMessages.length, 4)
        for (var i = 0; i < toastHost.activeMessages.length; ++i) {
            var message = toastHost.activeMessages[i]
            var point = message.mapToItem(toastHost, 0, 0)
            verify(message.width > 0 && message.height > 0, "Toast has invalid geometry")
            verify(point.x >= 0, "Toast starts outside the left edge")
            verify(point.x + message.width <= toastHost.width + 0.5,
                   "Toast extends outside the right edge")
            verify(point.y >= 0, "Toast starts above the host")
            verify(point.y + message.height <= toastHost.height + 0.5,
                   "Four-layer toast stack extends below the host")
        }
        verify(alertItem.x >= 0 && alertItem.x + alertItem.width <= stage.width,
               "Responsive alert extends outside the window")
        verify(alertItem.implicitHeight > 0, "Responsive alert has no content height")
    }

    function captureTheme(theme) {
        createFeedback(theme)
        verifyGeometry()
        var ok = Screenshot.capture(stage, "feedback_primitives_" + theme, harness.themeMode)
        verify(ok, "Failed to capture feedback primitives for " + theme)
        cleanup()
    }

    function test_feedback_light() {
        captureTheme("light")
    }

    function test_feedback_dark() {
        captureTheme("dark")
    }
}
