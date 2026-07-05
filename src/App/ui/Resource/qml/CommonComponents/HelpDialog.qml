import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 关于对话框:复用 GDialogShell 外壳(背景/阴影/进出动效/表头/分隔线)
GDialogShell {
    id: helpDialog
    width: 640
    height: 520

    title: qsTr("About GDownload")
    subtitle: qsTr("Version %1").arg(UtilsToolsManager.Version())
    iconImage: "/images/logo/icon.ico"
    iconBgColor: GTheme.infoLight(9)

    // ===== body:标签页导航 + 内容堆栈 =====
    ColumnLayout {
        anchors.fill: parent
        spacing: GTheme.spaceLG

        // 标签页导航
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.navItemHeight + GTheme.spaceXS
            Layout.topMargin: GTheme.spaceLG
            Layout.leftMargin: GTheme.space2XL
            Layout.rightMargin: GTheme.space2XL

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: [
                        { name: qsTr("Sponsorship"), icon: SegoeFluentIcons.Heart },
                        { name: qsTr("License"), icon: SegoeFluentIcons.Code },
                        { name: qsTr("About"), icon: SegoeFluentIcons.Info }
                    ]

                    GButton {
                        variant: "nav"
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: GTheme.navItemHeight
                        checkable: true
                        checked: index === tabNavigation.currentIndex
                        iconSource: modelData.icon
                        text: modelData.name
                        ButtonGroup.group: tabGroup
                        onClicked: {
                            tabNavigation.currentIndex = index
                        }
                    }
                }
            }
        }

        ButtonGroup {
            id: tabGroup
        }

        QtObject {
            id: tabNavigation
            property int currentIndex: 0
        }

        // 内容区域
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: GTheme.space2XL
            Layout.rightMargin: GTheme.space2XL
            Layout.bottomMargin: GTheme.spaceLG
            currentIndex: tabNavigation.currentIndex

            // 赞助页面
            GCard {
                outlined: true
                padding: GTheme.spaceLG

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceLG

                    Text {
                        text: qsTr("Support GDownload Development")
                        font.pixelSize: GTheme.fontSubtitle
                        font.weight: GTheme.weightMedium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    Text {
                        text: qsTr("If you like GDownload, you can support us through the following platforms:")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        Image {
                            fillMode: Image.PreserveAspectFit
                            source: "/payee/sponsor.jpg"
                            sourceSize.width: 400
                        }
                    }

                    Text {
                        text: qsTr("Thank you for your support! ❤️")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textPlaceholder
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // 许可证页面
            GCard {
                outlined: true
                padding: GTheme.spaceLG

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceSM

                    Text {
                        text: qsTr("Open Source Licenses")
                        font.pixelSize: GTheme.fontSubtitle
                        font.weight: GTheme.weightMedium
                        color: GTheme.textPrimary
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: UtilsToolsManager.GetNoticeContent()
                        font.pixelSize: GTheme.fontCaption
                    }
                }
            }

            // 关于页面
            GCard {
                outlined: true
                padding: GTheme.spaceLG

                ColumnLayout {
                    anchors.fill: parent
                    spacing: GTheme.spaceSM

                    Text {
                        text: qsTr("About GDownload")
                        font.pixelSize: GTheme.fontSubtitle
                        font.weight: GTheme.weightMedium
                        color: GTheme.textPrimary
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        textFormat: Text.MarkdownText
                        font.pixelSize: GTheme.fontBody
                        text: qsTr(`# GDownload

**A modern cross-platform download manager**

## Core Features

✅ **Multi-Platform Support** - Windows, macOS, Linux
✅ **High-Performance Engine** - Powered by aria2c
✅ **Multi-Protocol Support** - HTTP/HTTPS/FTP/BitTorrent/Metalink
✅ **Smart Download Management** - Multi-threaded, resume capability
✅ **Modern UI** - Built with Qt Quick and Element Plus design

## Technology Stack

- **Frontend**: Qt Quick (QML) + Element Plus Design
- **Backend**: Qt C++ + Modern C++20
- **Download Engine**: aria2c
- **Network**: Boost.Asio with SSL
- **BitTorrent**: LibtorrentRasterbar
- **Build System**: CMake + vcpkg

## Development Team

GDownload is maintained by passionate developers who believe in:
- Open source software
- Modern UI/UX design
- High-quality code
- Community collaboration

## Get Involved

🔗 **GitHub**: [https://github.com/cool2528/GDownload](https://github.com/cool2528/GDownload)
🐛 **Issues**: [Report bugs or request features](https://github.com/cool2528/GDownload/issues)
📝 **Contribute**: Pull requests are welcome!

## License

Copyright © 2024 GDownload Team
Licensed under the Apache License 2.0

---

*Thank you to all contributors and users who make GDownload better!* 🚀`)
                    }
                }
            }
        }
    }
}
