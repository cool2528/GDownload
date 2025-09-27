import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: control
    property alias index: browsesStack.currentIndex

    StackLayout {
        id: browsesStack
        anchors.fill: parent

        DownloadPageView {
            id: downloadPage
        }

        SettingsPageView {
            id: settingPage
        }
    }

    function switchDownloadPage(index: int) {
        downloadPage.currentIndex = index
    }

    function switchSettingPage(index: int) {
        settingPage.currentIndex = index
    }
}
