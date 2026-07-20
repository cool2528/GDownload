import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

        // Tab 栏（照 SettingsPageView 的 GButton nav + ButtonGroup 模式）
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: GTheme.spaceXL
            Layout.topMargin: GTheme.spaceMD
            spacing: GTheme.spaceSM

            GButton {
                id: searchTabButton
                objectName: "ed2kTabSearch"
                variant: "nav"
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
                checkable: true
                checked: ed2kStack.currentIndex === 1
                iconName: "globe"
                text: qsTr("Servers")
                ButtonGroup.group: ed2kTabGroup
                onClicked: ed2kStack.currentIndex = 1
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

            Ed2kSearchPage {
                objectName: "ed2kSearchPage"
                onGoToServers: ed2kStack.currentIndex = 1
            }
            // Task 7 替换为 Ed2kServerPage
            Item { objectName: "ed2kServersPlaceholder" }
        }
    }
}
