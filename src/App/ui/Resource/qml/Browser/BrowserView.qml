import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import gdl.sdk

Rectangle {
    id: control
    property alias index: browsesStack.currentIndex
    // 当前下载子页索引(0=Downloading 1=Waiting 2=Completed),供导航栏高亮当前页
    property alias downloadIndex: downloadPage.currentIndex
    readonly property string currentSectionTitle: {
        if (index === 1)
            return qsTr("Preferences")
        if (index === 2)
            return qsTr("Home")
        if (index === 3)
            return qsTr("eD2k")
        if (downloadIndex === 1)
            return qsTr("Waiting")
        if (downloadIndex === 2)
            return qsTr("Stopped")
        return qsTr("Downloads")
    }

    color: GTheme.bgPage

    StackLayout {
        id: browsesStack
        anchors.fill: parent
        // 默认启动显示主页(index 2:download=0 / settings=1 / home=2)
        currentIndex: 2

        DownloadPageView {
            id: downloadPage
            objectName: "downloadWorkspace"
        }

        SettingsPageView {
            id: settingPage
            objectName: "settingsWorkspace"
        }

        // 主页/概览仪表盘(导航 home 按钮 → index 2)
        HomePage {
            id: homePage
            objectName: "homeWorkspace"
        }

        // eD2k 中心页(导航 eD2k 按钮 → index 3)
        Ed2kCenterPage {
            id: ed2kPage
            objectName: "ed2kWorkspace"
        }
    }

    function switchDownloadPage(index: int) {
        downloadPage.currentIndex = index
    }

    function switchSettingPage(index: int) {
        settingPage.currentIndex = index
    }
}
