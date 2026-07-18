import QtQuick
import QtQuick.Layouts
import gdl.sdk 1.0
import "qrc:/qml/titlebar"
import "qrc:/qml/Navigator"
import "qrc:/qml/Browser"

Rectangle {
    id: mainWindow
    objectName: "applicationShellPreview"
    color: GTheme.bgPage

    property bool fullScreen: false
    property bool maximized: false
    property bool active: true

    function hide() {}
    function showMinimized2() {}
    function toggleFullScreen() { fullScreen = !fullScreen }
    function toggleMaximized() { maximized = !maximized }

    QtObject {
        id: helper
        function setHitTestVisible(item) {}
    }

    TitleBar {
        id: titleBar
        visible: true
        height: GTheme.titleBarHeight
        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        sectionTitle: brower_view.currentSectionTitle
        windowActive: mainWindow.active
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.bottom: parent.bottom
        spacing: 0

        NavigatorView {
            Layout.preferredWidth: GTheme.navBarWidth
            Layout.minimumWidth: GTheme.navBarWidth
            Layout.maximumWidth: GTheme.navBarWidth
            Layout.fillHeight: true
        }

        BrowserView {
            id: brower_view
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
