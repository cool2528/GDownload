import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格下载页面视图
Item {
    id: control
    property alias currentIndex: navigationBar.currentIndex

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int standardRadius: 4
    readonly property int sidebarWidth: 240

    RowLayout {
        id: browserLayout
        anchors.fill: parent
        spacing: 0

        // 左侧导航栏
        Rectangle {
            id: leftSidebar
            color: GTheme.bgPage
            Layout.fillHeight: true
            Layout.minimumWidth: control.sidebarWidth
            Layout.preferredWidth: control.sidebarWidth
            Layout.maximumWidth: control.sidebarWidth

            // 右侧分隔线
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: GTheme.borderLight
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: control.standardSpacing
                spacing: control.standardSpacing

                // 页面标题
                Text {
                    text: qsTr("Download Tasks")
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: GTheme.textPrimary
                    Layout.fillWidth: true
                    Layout.bottomMargin: 8
                }

                // 导航按钮组
                ColumnLayout {
                    id: navigationBar
                    Layout.fillWidth: true
                    spacing: 4
                    property int currentIndex: 0
                    property var buttonsArr: [downloadingBtn, waitingBtn, stoppedBtn]

                    ButtonGroup {
                        id: navigationGroup
                        onCheckedButtonChanged: {
                            let index = navigationBar.buttonsArr.indexOf(navigationGroup.checkedButton)
                            navigationBar.currentIndex = index
                            console.debug("Navigation index:", index)
                        }
                    }

                    // 下载中按钮
                    GNavButton {
                        id: downloadingBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        checkable: true
                        checked: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.PlaySolid
                        text: qsTr("Downloading")
                        onClicked: {
                            checked = true
                        }
                    }

                    // 等待中按钮
                    GNavButton {
                        id: waitingBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        checkable: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.PauseBold
                        text: qsTr("Waiting")
                        onClicked: {
                            checked = true
                        }
                    }

                    // 已停止按钮
                    GNavButton {
                        id: stoppedBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        checkable: true
                        ButtonGroup.group: navigationGroup
                        iconSource: SegoeFluentIcons.Stop
                        text: qsTr("Stopped")
                        onClicked: {
                            checked = true
                        }
                    }

                    // 填充空间
                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }

        // 主内容区域
        Rectangle {
            id: mainContent
            color: GTheme.bgWhite
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 400

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 页面标题栏
                DownloadPageTitle {
                    id: downloadTitle
                    Layout.fillWidth: true
                    type: navigationBar.currentIndex
                }

                // 下载列表堆栈视图
                StackLayout {
                    id: downloadStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: navigationBar.currentIndex

                    // 下载中页面
                    GDownloadViewPage {
                        id: downloadPage
                        pageType: 0
                        objectName: "downloadPage"
                        model: BrowserManager.GetActiveDownloadModel()
                    }

                    // 等待中页面
                    GDownloadViewPage {
                        id: waitingPage
                        pageType: 1
                        objectName: "waitingPage"
                        model: BrowserManager.GetWaitingDownloadModel()
                    }

                    // 已停止页面
                    GDownloadViewPage {
                        id: completedPage
                        pageType: 2
                        objectName: "completedPage"
                        model: BrowserManager.GetStopedDownloadModel()
                    }
                }
            }
        }
    }
}
