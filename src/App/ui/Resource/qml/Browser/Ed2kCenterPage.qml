import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// eD2k 中心页：搜索 / 服务器 两个子页，右上角常驻连接状态徽标
Rectangle {
    id: control
    objectName: "ed2kWorkspace"
    color: GTheme.bgPage

    property alias currentTabIndex: ed2kStack.currentIndex

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 标题区（参照 DownloadPageTitle 风格）
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 88
            color: GTheme.bgPage

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: GTheme.spaceXL
                anchors.rightMargin: GTheme.spaceXL

                ColumnLayout {
                    spacing: GTheme.spaceXS
                    Text {
                        text: qsTr("eD2k Network")
                        font.pixelSize: GTheme.fontH1
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                    }
                    Text {
                        text: qsTr("Search files and manage servers on the eD2k network")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }
                }

                Item { Layout.fillWidth: true }

                // 连接状态徽标：HighID 绿 / LowID 黄 / 未连接灰
                Rectangle {
                    objectName: "ed2kConnectionBadge"
                    radius: GTheme.radiusRound
                    color: Ed2kManager.serverConnected
                           ? (Ed2kManager.highId ? GTheme.bgSuccess : GTheme.bgWarning)
                           : GTheme.fillLight
                    implicitHeight: 28
                    implicitWidth: badgeRow.implicitWidth + GTheme.spaceMD * 2

                    RowLayout {
                        id: badgeRow
                        anchors.centerIn: parent
                        spacing: GTheme.spaceXS
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: Ed2kManager.serverConnected
                                   ? (Ed2kManager.highId ? GTheme.successColor : GTheme.warningColor)
                                   : GTheme.textPlaceholder
                        }
                        Text {
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textRegular
                            text: Ed2kManager.serverConnected
                                  ? (Ed2kManager.connectedServerName.length > 0
                                     ? qsTr("Connected: %1").arg(Ed2kManager.connectedServerName)
                                     : qsTr("Connected"))
                                  : qsTr("Not connected")
                        }
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: GTheme.borderLighter
            }
        }

        // 引擎不可用占位态：eD2k 引擎启动失败时，隐藏 Tab 栏与内容栈，仅保留标题区
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !Ed2kManager.engineAvailable

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - GTheme.spaceXL * 2, 480)
                spacing: GTheme.spaceSM

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: GTheme.fontTitle
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    text: qsTr("eD2k engine is unavailable")
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textSecondary
                    text: qsTr("The eD2k engine failed to start. Check the logs or restart the app.")
                }
            }
        }

        // Tab 栏（照 SettingsPageView 的 GButton nav + ButtonGroup 模式）
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: GTheme.spaceXL
            Layout.topMargin: GTheme.spaceMD
            spacing: GTheme.spaceSM
            visible: Ed2kManager.engineAvailable

            GButton {
                id: searchTabButton
                objectName: "ed2kTabSearch"
                variant: "nav"
                // nav 变体的文本区宽度与隐式宽度精确贴边，高分屏/中文字体度量的
                // 浮点取整会触发省略号——三个 tab 按钮统一加一点余量
                Layout.preferredWidth: implicitWidth + GTheme.spaceSM
                checkable: true
                checked: ed2kStack.currentIndex === 0
                // AuroraIcons 中没有 "search" 图标，用语义相近的 "filter" 代替（不新增 SVG）
                iconName: "filter"
                text: qsTr("Search")
                ButtonGroup.group: ed2kTabGroup
                onClicked: ed2kStack.currentIndex = 0
            }
            GButton {
                id: serversTabButton
                objectName: "ed2kTabServers"
                variant: "nav"
                Layout.preferredWidth: implicitWidth + GTheme.spaceSM
                checkable: true
                checked: ed2kStack.currentIndex === 1
                iconName: "globe"
                text: qsTr("Servers")
                ButtonGroup.group: ed2kTabGroup
                onClicked: ed2kStack.currentIndex = 1
            }
            GButton {
                id: sharesTabButton
                objectName: "ed2kTabShares"
                variant: "nav"
                Layout.preferredWidth: implicitWidth + GTheme.spaceSM
                checkable: true
                checked: ed2kStack.currentIndex === 2
                // AuroraIcons 中没有 "share" 图标，用语义相近的 "cloud" 代替（不新增 SVG）
                iconName: "cloud"
                text: qsTr("Shares")
                ButtonGroup.group: ed2kTabGroup
                onClicked: ed2kStack.currentIndex = 2
            }
            Item { Layout.fillWidth: true }
        }
        ButtonGroup { id: ed2kTabGroup }

        StackLayout {
            id: ed2kStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: GTheme.spaceXL
            currentIndex: 0
            visible: Ed2kManager.engineAvailable

            Ed2kSearchPage {
                objectName: "ed2kSearchPage"
                onGoToServers: ed2kStack.currentIndex = 1
            }
            Ed2kServerPage {
                objectName: "ed2kServerPage"
            }
            Ed2kSharePage {
                objectName: "ed2kSharePage"
            }
        }
    }
}
