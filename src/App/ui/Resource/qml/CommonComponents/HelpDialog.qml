import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
Popup{
    id: helpDialog
    width: 580
    height: 450
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 0
    modal: true
    visible: false
    closePolicy: Popup.NoAutoClose
    contentItem: Rectangle{
        anchors.fill: parent
        // 头部
        color: "transparent"
        clip: true
        RowLayout{
            id:headerLogo
            anchors{
                top: parent.top
                topMargin: 20
                left: parent.left
                leftMargin: 30
                right: parent.right
                rightMargin: 20
            }
            width: parent.width
            height: 30
            spacing: 10
            Image {
                id: logo
                anchors{
                    left: parent.left
                    verticalCenter: parent.verticalCenter

                }
                Layout.preferredHeight: 16
                Layout.preferredWidth: 16
                source: "/images/logo/icon.ico"
            }
            Label{
                text: String("GDownloader Version: %1").arg(UtilsToolsManager.Version())
                font.pixelSize: 15
                color: GTheme.dark ?  "#ffffff" : "#3b3b3b"
                verticalAlignment: Text.AlignVCenter
            }
            ImageButton{
                id:closeBtn
                Layout.alignment: Qt.AlignRight
                Layout.preferredHeight: 15
                Layout.preferredWidth: 15
                backgroundColor:"transparent"
                normalImage: "/images/dialog/dialog_close_normol.svg"
                hoverImage: "/images/dialog/dialog_close_hover.svg"
                onClicked: {
                    helpDialog.close()
                }
            }
        }
        // 内容
        ListView{
            id:tabArea
            anchors{
                top: headerLogo.bottom
                topMargin: 20
                left: parent.left
                leftMargin: 20
            }
            width: contentWidth
            height: 30
            layoutDirection: Qt.LeftToRight
            orientation: ListView.Horizontal
            boundsBehavior: Flickable.StopAtBounds
            spacing: 10
            clip: true
            model: [qsTr("Sponsorship"),qsTr("License"),qsTr("About")]
            delegate:Rectangle{
                height: parent.height
                width: text.contentWidth + 20
                radius: 5
                color: tabArea.currentIndex === index || parent.hovered ? (GTheme.dark ? "#5151f9" : "#f2f2f2") : "transparent"
                Button {
                    anchors.verticalCenter: parent.verticalCenter
                    contentItem: Text {
                        id: text
                        anchors.fill: parent
                        text: modelData
                        font.pixelSize: 14
                        color: tabArea.currentIndex === index  || parent.hovered ? (GTheme.dark ? "#ffffff" : "#5151f9") : (GTheme.dark ? "#a0a0a0" : "#494c55" )
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: MouseArea {
                        cursorShape: Qt.PointingHandCursor;
                        acceptedButtons: Qt.NoButton
                    }
                    onClicked: {
                        tabArea.currentIndex = index;
                    }
                }
            }
        }
        Rectangle{
            id:splitLine
            anchors.bottom: tabArea.bottom
            anchors.bottomMargin: -5
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: parent.width
            height: 3
            color: GTheme.dark ? "#434343" : "#e5e5e5"
        }
        StackLayout{
            id:layout
            anchors.top: splitLine.bottom
            anchors.left: tabArea.left
            width: parent.width - 20
            height: 325
            focus: true
            currentIndex: tabArea.currentIndex
            // Sponsor page
            Item{
                id: sponsor
                Layout.fillWidth: true
                Layout.rightMargin: 10
                ColumnLayout{
                    anchors.fill: parent
                    spacing: 10
                    Label{
                        text: qsTr("If you like GDownloader, you can sponsor us on the following platforms:")
                        font.pixelSize: 14
                        color: GTheme.dark ?  "#ffffff" : "#3b3b3b"
                        Layout.margins: 10
                    }
                    Image {
                        fillMode: Image.PreserveAspectFit
                        source: "/payee/sponsor.jpg"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 10
                    }
                }
            }

            // Open Source License page
            ScrollText{
                id: license
                Layout.rightMargin: 10
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: UtilsToolsManager.GetNoticeContent()
            }

            // About page
            ScrollText{
                id: about
                Layout.rightMargin: 10
                Layout.fillWidth: true
                Layout.fillHeight: true
                textFormat: Text.MarkdownText
                text:qsTr('# About GDownload

GDownload is a cross-platform download manager built with C++ and Qt. It combines modern technology stack and excellent open-source components to provide users with an efficient and stable downloading experience.

## Core Features

- Cross-platform support (Windows, macOS, Linux)
- Efficient download engine powered by aria2c
- Multi-threaded concurrent downloads
- Support for multiple protocols (HTTP, HTTPS, FTP, BitTorrent, Metalink)
- Download resume capability
- User-friendly graphical interface

## Technology Stack

- UI Framework: Qt Quick (QML) + Qt C++
- Core Engine: aria2c
- Network Library: Boost.Asio
- BT Download: LibtorrentRasterbar
- XML Parser: PugiXML
- Frameless Window: FramelessHelper

## Development Team

GDownload is an open-source project maintained by developers who are passionate about technology. We welcome community contributions, including but not limited to:

- Code contributions
- Bug reports
- Feature suggestions
- Documentation improvements

## Contact Us

- [GitHub:](https://github.com/cool2528/GDownload)
- [Issue Tracking:](https://github.com/cool2528/GDownload/issues)
- [Home Page: ](https://github.com/cool2528/GDownload)

## Copyright Notice

Copyright © 2024 GDownload Team
Licensed under the Apache License 2.0

*Thanks to all developers and users who have contributed to this project!*')
            }
        }

    }
    background: Rectangle{
        color: GTheme.dark ?  "#2e2e2e" : "#ffffff"
        radius: 5
    }

}
