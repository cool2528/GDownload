import QtQuick
import gdl.sdk 1.0
import "qrc:/qml/Navigator"

Rectangle {
    id: mainWindow
    color: GTheme.bgPage

    function addDownloadTask() { return null }
    function showAboutDialog() { return null }

    QtObject {
        id: brower_view
        property int index: 2
        property int downloadIndex: 0
        function switchDownloadPage(index) { downloadIndex = index }
        function switchSettingPage(index) {}
    }

    NavigatorView {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: GTheme.navBarWidth
    }
}
