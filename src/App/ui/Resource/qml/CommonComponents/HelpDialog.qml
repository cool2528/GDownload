import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 关于与许可证共用现有对话框，并在窄窗口中纵向排列标签页。
GDialogShell {
    id: helpDialog
    objectName: "helpDialog"

    readonly property real outerMargin: GTheme.spaceLG
    readonly property bool narrowLayout: width < 500

    width: Math.min(640, parent ? Math.max(0, parent.width - outerMargin * 2) : 640)
    height: Math.min(540, parent ? Math.max(0, parent.height - outerMargin * 2) : 540)

    title: qsTr("About GDownload")
    subtitle: qsTr("Version %1").arg(UtilsToolsManager.Version())
    iconName: "help"
    iconBgColor: GTheme.bgInfo
    iconColor: GTheme.infoColor

    ColumnLayout {
        anchors.fill: parent
        spacing: GTheme.spaceLG

        GridLayout {
            id: helpTabs
            objectName: "helpTabs"
            Layout.fillWidth: true
            Layout.topMargin: GTheme.spaceLG
            Layout.leftMargin: helpDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
            Layout.rightMargin: helpDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
            columns: helpDialog.narrowLayout ? 1 : 2
            rowSpacing: GTheme.spaceSM
            columnSpacing: GTheme.spaceSM

            Repeater {
                objectName: "helpTabRepeater"
                model: [
                    { name: qsTr("License"), icon: "code" },
                    { name: qsTr("About"), icon: "info" }
                ]

                GButton {
                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    checkable: true
                    checked: index === tabNavigation.currentIndex
                    iconName: modelData.icon
                    text: modelData.name
                    ButtonGroup.group: tabGroup
                    Accessible.name: text
                    Accessible.description: checked ? qsTr("Selected tab") : qsTr("Open tab")
                    onClicked: tabNavigation.currentIndex = index
                    Keys.onReturnPressed: event => {
                        tabNavigation.currentIndex = index
                        event.accepted = true
                    }
                    Keys.onEnterPressed: event => {
                        tabNavigation.currentIndex = index
                        event.accepted = true
                    }
                }
            }
        }

        ButtonGroup {
            id: tabGroup
        }

        QtObject {
            id: tabNavigation
            objectName: "helpTabNavigation"
            property int currentIndex: 0
        }

        StackLayout {
            objectName: "helpPages"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: helpDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
            Layout.rightMargin: helpDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
            Layout.bottomMargin: GTheme.spaceLG
            currentIndex: tabNavigation.currentIndex

            GCard {
                outlined: true
                interactive: false
                hoverEnabled: false
                padding: helpDialog.narrowLayout ? GTheme.spaceMD : GTheme.spaceLG

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceMD

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceMD

                        AuroraIcon {
                            name: "code"
                            iconSize: GTheme.fontTitle
                            color: GTheme.primaryColor
                        }

                        Text {
                            text: qsTr("Open Source Licenses")
                            font.pixelSize: GTheme.fontSubtitle
                            font.weight: GTheme.weightDemiBold
                            color: GTheme.textPrimary
                            Layout.fillWidth: true
                        }
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: UtilsToolsManager.GetNoticeContent()
                        font.pixelSize: GTheme.fontCaption
                        Accessible.name: qsTr("Open source license notices")
                    }
                }
            }

            GCard {
                outlined: true
                interactive: false
                hoverEnabled: false
                padding: helpDialog.narrowLayout ? GTheme.spaceMD : GTheme.spaceLG

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceMD

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceMD

                        AuroraBrand {
                            variant: "monochrome"
                            markSize: GTheme.sizeLarge
                        }

                        Text {
                            text: qsTr("About GDownload")
                            font.pixelSize: GTheme.fontSubtitle
                            font.weight: GTheme.weightDemiBold
                            color: GTheme.textPrimary
                            Layout.fillWidth: true
                        }
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        textFormat: Text.MarkdownText
                        font.pixelSize: GTheme.fontBody
                        Accessible.name: qsTr("About GDownload")
                        text: qsTr(`# GDownload

**A modern cross-platform download manager**

## Core Features

- Multi-platform support for Windows, macOS, and Linux
- High-performance downloads powered by aria2c
- HTTP, HTTPS, FTP, BitTorrent, and Metalink support
- Multi-threaded downloads with resume capability
- A native Qt Quick desktop experience

## Technology

- **Frontend**: Qt Quick (QML)
- **Backend**: Qt and modern C++20
- **Download engine**: aria2c
- **Network**: Boost.Asio with SSL
- **BitTorrent**: libtorrent
- **Build system**: CMake and vcpkg

## Get Involved

- [GitHub](https://github.com/cool2528/GDownload)
- [Report an issue](https://github.com/cool2528/GDownload/issues)
- Pull requests are welcome

## License

Copyright © 2024-2026 GDownload Team

Licensed under the GNU Affero General Public License v3.0 (AGPL-3.0). This is free software, and you are welcome to redistribute it under certain conditions. It comes with ABSOLUTELY NO WARRANTY.`)
                    }
                }
            }
        }
    }

}
