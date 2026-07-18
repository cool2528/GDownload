import QtQuick
import QtTest
import gdl.sdk
import "qrc:/tests/qml_ui/support"
import "qrc:/qml/CommonComponents"

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
    property var currentPreview: null

    Component {
        id: torrentPreviewComponent

        Rectangle {
            id: previewHost

            property int previewWidth: 520
            property alias previewItem: filePreview

            width: previewWidth + 96
            height: 470
            color: GTheme.bgPage

            ListModel {
                id: torrentFiles
                property int selectedCount: 2
                property string totalSize: "4.8 GB"

                function selectAll() {
                    for (var i = 0; i < count; ++i)
                        setProperty(i, "isSelected", true)
                    selectedCount = count
                }
                function unselectAll() {
                    for (var i = 0; i < count; ++i)
                        setProperty(i, "isSelected", false)
                    selectedCount = 0
                }
                function toggleSelection(index) {
                    var selected = !get(index).isSelected
                    setProperty(index, "isSelected", selected)
                    selectedCount += selected ? 1 : -1
                }

                ListElement {
                    fileName: "Aurora.Design.System.Reference.2026.Final.Release.iso"
                    fileExtension: ".iso"
                    fileSize: "4.7 GB"
                    isSelected: true
                }
                ListElement {
                    fileName: "Documentation and implementation notes.pdf"
                    fileExtension: ".pdf"
                    fileSize: "86 MB"
                    isSelected: true
                }
                ListElement {
                    fileName: "Optional source samples and screenshots.zip"
                    fileExtension: ".zip"
                    fileSize: "24 MB"
                    isSelected: false
                }
            }

            Column {
                anchors.centerIn: parent
                width: previewHost.previewWidth
                spacing: GTheme.spaceMD

                GDropArea {
                    id: dropArea
                    objectName: "torrentDropArea"
                    width: parent.width
                    height: 150
                }

                FilePreviewList {
                    id: filePreview
                    objectName: "torrentFilePreview"
                    width: parent.width
                    height: 250
                    previewModel: torrentFiles
                }
            }
        }
    }

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
        if (currentPreview !== null) {
            currentPreview.destroy()
            currentPreview = null
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

    function createTaskDialog(theme, width, height, initialTab) {
        testCase.width = width
        testCase.height = height
        harness.themeMode = theme
        wait(120)
        var comp = Qt.createComponent("qrc:/qml/CommonComponents/TaskDialogPage.qml")
        verify(comp.status === Component.Ready, comp.errorString())
        var popup = comp.createObject(harness, { initialTab: initialTab })
        verify(popup !== null)
        currentPopup = popup
        popup.parent = harness
        popup.open()
        wait(350)
        return popup
    }

    function test_taskdialog_narrow_validation_light() {
        var popup = createTaskDialog("light", 420, 760, 0)
        verify(popup.x >= 0)
        verify(popup.x + popup.width <= harness.width)
        var addButton = findChild(popup, "btnCreateTask")
        var cancelButton = findChild(popup, "btnCancelTask")
        verify(addButton !== null)
        verify(cancelButton !== null)
        mouseClick(addButton)
        wait(150)
        var alert = findChild(popup, "taskDialogValidationAlert")
        verify(alert !== null)
        verify(alert.visible)
        verify(Screenshot.captureWindow(harness, "taskdialog_url_validation_narrow_light", "light"))
        cleanupPopup()
    }

    function test_taskdialog_torrent_advanced_narrow_dark() {
        var popup = createTaskDialog("dark", 520, 820, 1)
        var dropArea = findChild(popup, "taskTorrentDropArea")
        var advancedToggle = findChild(popup, "taskAdvancedOptionsToggle")
        verify(dropArea !== null)
        verify(advancedToggle !== null)
        advancedToggle.checked = true
        wait(250)
        var scroll = findChild(popup, "taskDialogScroll")
        verify(scroll !== null)
        scroll.contentItem.contentY = Math.max(0, scroll.contentItem.contentHeight - scroll.height)
        wait(200)
        verify(Screenshot.captureWindow(harness, "taskdialog_torrent_advanced_narrow_dark", "dark"))
        cleanupPopup()
    }

    function test_taskdialog_torrent_advanced_narrow_light() {
        var popup = createTaskDialog("light", 520, 820, 1)
        var advancedToggle = findChild(popup, "taskAdvancedOptionsToggle")
        verify(advancedToggle !== null)
        advancedToggle.checked = true
        wait(250)
        var scroll = findChild(popup, "taskDialogScroll")
        verify(scroll !== null)
        scroll.contentItem.contentY = Math.max(0, scroll.contentItem.contentHeight - scroll.height)
        wait(200)
        verify(Screenshot.captureWindow(harness, "taskdialog_torrent_advanced_narrow_light", "light"))
        cleanupPopup()
    }

    function test_taskdialog_baidu_narrow_light() {
        var popup = createTaskDialog("light", 520, 820, 2)
        verify(findChild(popup, "netDiskPage") !== null)
        verify(Screenshot.captureWindow(harness, "taskdialog_baidu_narrow_light", "light"))
        cleanupPopup()
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

    function captureTorrentPreview(tag, theme, previewWidth) {
        testCase.width = previewWidth + 160
        testCase.height = 540
        harness.themeMode = theme
        harness.load("")
        wait(100)
        currentPreview = torrentPreviewComponent.createObject(harness, { previewWidth: previewWidth })
        verify(currentPreview !== null, "Failed to create torrent preview")
        currentPreview.x = Math.round((harness.width - currentPreview.width) / 2)
        currentPreview.y = Math.round((harness.height - currentPreview.height) / 2)
        wait(250)
        compare(currentPreview.previewItem.width, previewWidth)
        verify(currentPreview.previewItem.x >= 0)
        verify(currentPreview.previewItem.x + currentPreview.previewItem.width <= currentPreview.width)
        var ok = Screenshot.capture(currentPreview, tag, harness.themeMode)
        verify(ok, "Screenshot.capture failed for " + tag)
    }

    function test_torrent_preview_compact_light() {
        captureTorrentPreview("torrent_preview_compact_light", "light", 520)
    }

    function test_torrent_preview_wide_dark() {
        captureTorrentPreview("torrent_preview_wide_dark", "dark", 720)
    }

    function test_netdisk_workflow_light() {
        loadPageAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_workflow_light", "light", 1180, 720)
    }

    function test_netdisk_workflow_dark() {
        loadPageAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_workflow_dark", "dark", 1180, 720)
    }

    function test_netdisk_workflow_narrow_geometry() {
        testCase.width = 420
        testCase.height = 700
        harness.themeMode = "light"
        harness.load("qrc:/qml/CommonComponents/NetDiskPageView.qml")
        wait(300)

        var page = harness.loadedItem
        verify(page !== null)
        var input = findChild(page, "netDiskUrlInput")
        var button = findChild(page, "netDiskParseButton")
        var viewport = findChild(page, "netDiskParseViewport")
        verify(input !== null)
        verify(button !== null)
        verify(viewport !== null)
        verify(input.x >= 0)
        verify(input.x + input.width <= page.width)
        verify(button.x >= 0)
        verify(button.x + button.width <= page.width)
        compare(viewport.contentWidth, viewport.width)

        var ok = Screenshot.capture(harness, "netdisk_workflow_narrow_light", harness.themeMode)
        verify(ok, "Screenshot.capture failed for netdisk_workflow_narrow_light")
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
