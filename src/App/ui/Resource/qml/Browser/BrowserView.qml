import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: control
    property alias index: browsesStack.currentIndex
    // 当前下载子页索引(0=Downloading 1=Waiting 2=Completed),供导航栏高亮当前页
    property alias downloadIndex: downloadPage.currentIndex

    StackLayout {
        id: browsesStack
        anchors.fill: parent
        // 默认启动显示主页(index 2:download=0 / settings=1 / home=2)
        currentIndex: 2

        DownloadPageView {
            id: downloadPage
        }

        SettingsPageView {
            id: settingPage
        }

        // 主页/概览仪表盘(导航 home 按钮 → index 2)
        HomePage {
            id: homePage
        }
    }

    function switchDownloadPage(index: int) {
        downloadPage.currentIndex = index
    }

    function switchSettingPage(index: int) {
        settingPage.currentIndex = index
    }
}
