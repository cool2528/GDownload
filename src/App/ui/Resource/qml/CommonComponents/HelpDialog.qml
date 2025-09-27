import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

// Element Plus 风格帮助对话框
Popup {
    id: helpDialog
    width: 640
    height: 520
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    visible: false
    closePolicy: Popup.CloseOnEscape

    // Element Plus 设计标准
    readonly property int standardPadding: 24
    readonly property int standardSpacing: 16
    readonly property int headerHeight: 64

    // Element Plus 风格背景
    background: Rectangle {
        color: GTheme.bgWhite
        radius: 8
        border.width: 1
        border.color: GTheme.borderLight

        // Element Plus 风格阴影
        layer.enabled: true
        layer.effect: DropShadow {
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.1)
            horizontalOffset: 0
            verticalOffset: 4
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // 头部区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: helpDialog.headerHeight
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: helpDialog.standardPadding
                anchors.rightMargin: helpDialog.standardPadding
                spacing: helpDialog.standardSpacing

                // 图标区域
                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    Layout.alignment: Qt.AlignVCenter
                    color: GTheme.infoLight(9)
                    radius: 8

                    Image {
                        anchors.centerIn: parent
                        source: "/images/logo/icon.ico"
                        sourceSize: Qt.size(24, 24)
                    }
                }

                // 标题和版本信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        text: qsTr("About GDownload")
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        color: GTheme.textPrimary
                    }

                    Text {
                        text: String("Version %1").arg(UtilsToolsManager.Version())
                        font.pixelSize: 14
                        color: GTheme.textSecondary
                    }
                }

                // 关闭按钮
                IconButton {
                    iconSource: SegoeFluentIcons.ChromeClose
                    iconSize: 14
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    iconColor: hovered ? GTheme.textPrimary : GTheme.textSecondary
                    backgroundColor: hovered ? GTheme.fillLight : "transparent"
                    onClicked: helpDialog.close()
                }
            }
        }

        // 分隔线
        Divider {
            Layout.fillWidth: true
        }

        // 标签页导航
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.topMargin: helpDialog.standardSpacing
            Layout.leftMargin: helpDialog.standardPadding
            Layout.rightMargin: helpDialog.standardPadding
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: [
                        { name: qsTr("Sponsorship"), icon: SegoeFluentIcons.Heart },
                        { name: qsTr("License"), icon: SegoeFluentIcons.FileText },
                        { name: qsTr("About"), icon: SegoeFluentIcons.Info }
                    ]

                    GNavButton {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
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

            ButtonGroup {
                id: tabGroup
            }

            QtObject {
                id: tabNavigation
                property int currentIndex: 0
            }
        }

        // 内容区域
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: helpDialog.standardPadding
            Layout.rightMargin: helpDialog.standardPadding
            Layout.bottomMargin: helpDialog.standardSpacing
            currentIndex: tabNavigation.currentIndex

            // 赞助页面
            GCard {
                outlined: true
                padding: helpDialog.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    spacing: helpDialog.standardSpacing

                    Text {
                        text: qsTr("Support GDownload Development")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    Text {
                        text: qsTr("If you like GDownload, you can support us through the following platforms:")
                        font.pixelSize: 14
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
                        font.pixelSize: 12
                        color: GTheme.textPlaceholder
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // 许可证页面
            GCard {
                outlined: true
                padding: helpDialog.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Text {
                        text: qsTr("Open Source Licenses")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: UtilsToolsManager.GetNoticeContent()
                        font.pixelSize: 12
                    }
                }
            }

            // 关于页面
            GCard {
                outlined: true
                padding: helpDialog.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Text {
                        text: qsTr("About GDownload")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                    }

                    ScrollText {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        textFormat: Text.MarkdownText
                        font.pixelSize: 13
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

    // 打开动画
    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 0.9
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.OutCubic
            }
        }
    }

    // 关闭动画
    exit: Transition {
        ParallelAnimation {
            NumberAnimation {
                property: "scale"
                from: 1.0
                to: 0.9
                duration: 150
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 150
                easing.type: Easing.InCubic
            }
        }
    }
}
