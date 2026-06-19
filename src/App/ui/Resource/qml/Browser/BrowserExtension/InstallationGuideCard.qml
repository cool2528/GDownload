import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 安装指南卡片 - 分步骤引导用户安装浏览器插件
GCard {
    id: installCard
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 20

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("Installation Guide")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Follow these simple steps to get started")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // Step 1: Download Extension
        StepSection {
            Layout.fillWidth: true
            stepNumber: 1
            stepTitle: qsTr("Download Extension")
            stepDescription: qsTr("Choose your browser and download the extension")

            stepContent: ColumnLayout {
                spacing: 12

                Text {
                    text: qsTr("Select your browser:")
                    font.pixelSize: 13
                    color: GTheme.textRegular
                }

                // 浏览器按钮行
                RowLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    // Chrome 按钮
                    BrowserButton {
                        browserName: "Chrome"
                        iconSource: "qrc:/images/browser-extension/chrome.svg"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension/releases")
                            ToastManager.ShowInfo(qsTr("Opening Chrome download page..."), 2000)
                        }
                    }

                    // Firefox 按钮
                    BrowserButton {
                        browserName: "Firefox"
                        iconSource: "qrc:/images/browser-extension/firefox.svg"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension/releases")
                            ToastManager.ShowInfo(qsTr("Opening Firefox download page..."), 2000)
                        }
                    }

                    // Edge 按钮
                    BrowserButton {
                        browserName: "Edge"
                        iconSource: "qrc:/images/browser-extension/edge.svg"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension/releases")
                            ToastManager.ShowInfo(qsTr("Opening Edge download page..."), 2000)
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // 分隔线
                Divider {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: 4
                    color: GTheme.borderLight
                }

                // 其他下载方式
                RowLayout {
                    spacing: 20
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: 8

                        Text {
                            text: "\uE943"  // code/github icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 16
                            color: GTheme.textSecondary
                        }

                        Text {
                            text: qsTr("GitHub Release:")
                            font.pixelSize: 12
                            color: GTheme.textRegular
                        }

                        Text {
                            text: qsTr("Visit Repository")
                            font.pixelSize: 12
                            color: GTheme.primaryColor
                            font.underline: linkHover1.hovered

                            HoverHandler { id: linkHover1 }
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension")
                                    ToastManager.ShowInfo(qsTr("Opening GitHub..."), 2000)
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 8

                        Text {
                            text: "\uE774"  // globe icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 16
                            color: GTheme.textSecondary
                        }

                        Text {
                            text: qsTr("Official Site:")
                            font.pixelSize: 12
                            color: GTheme.textRegular
                        }

                        Text {
                            text: qsTr("Download from gdownload.uk")
                            font.pixelSize: 12
                            color: GTheme.primaryColor
                            font.underline: linkHover2.hovered

                            HoverHandler { id: linkHover2 }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    Qt.openUrlExternally("https://gdownload.uk/")
                                    ToastManager.ShowInfo(qsTr("Opening official website..."), 2000)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Step 2: Install Extension
        StepSection {
            Layout.fillWidth: true
            stepNumber: 2
            stepTitle: qsTr("Install Extension")
            stepDescription: qsTr("Load the extension in your browser")

            stepContent: ColumnLayout {
                spacing: 12

                // Chrome/Edge 安装说明
                InstallInstructionItem {
                    Layout.fillWidth: true
                    browserIcon: "\uE774"  // browser/globe icon
                    browserName: qsTr("For Chrome/Edge:")
                    instructions: [
                        qsTr("1. Open chrome://extensions/ (or edge://extensions/)"),
                        qsTr("2. Enable \"Developer mode\" toggle"),
                        qsTr("3. Click \"Load unpacked\""),
                        qsTr("4. Select the dist folder from extracted files")
                    ]
                }

                // Firefox 安装说明
                InstallInstructionItem {
                    Layout.fillWidth: true
                    browserIcon: "\uE774"  // browser/globe icon
                    browserName: qsTr("For Firefox:")
                    instructions: [
                        qsTr("1. Open about:debugging#/runtime/this-firefox"),
                        qsTr("2. Click \"Load Temporary Add-on\""),
                        qsTr("3. Select manifest.json from the dist folder"),
                        qsTr("4. Extension will be loaded temporarily")
                    ]
                }

                // 提示信息
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: tipLayout.implicitHeight + 16
                    color: Qt.rgba(GTheme.primaryColor.r, GTheme.primaryColor.g, GTheme.primaryColor.b, 0.1)
                    radius: 4

                    RowLayout {
                        id: tipLayout
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "\uEA80"  // lightbulb icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 16
                            color: GTheme.primaryColor
                        }

                        Text {
                            text: qsTr("Coming Soon: Direct installation from browser web stores!")
                            font.pixelSize: 12
                            color: GTheme.textRegular
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        // Step 3: Configure Connection
        StepSection {
            Layout.fillWidth: true
            stepNumber: 3
            stepTitle: qsTr("Configure Connection")
            stepDescription: qsTr("Set up the connection to GDownload")

            stepContent: ColumnLayout {
                spacing: 12

                Text {
                    text: qsTr("The extension needs to connect to GDownload's aria2c:")
                    font.pixelSize: 13
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                // 配置说明列表
                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    Row {
                        spacing: 8
                        Text {
                            text: "✅"
                            font.pixelSize: 14
                        }
                        Text {
                            text: qsTr("Default settings are pre-configured")
                            font.pixelSize: 12
                            color: GTheme.textRegular
                        }
                    }

                    Row {
                        spacing: 8
                        Text {
                            text: "✅"
                            font.pixelSize: 14
                        }
                        Text {
                            text: qsTr("Works out-of-the-box with GDownload")
                            font.pixelSize: 12
                            color: GTheme.textRegular
                        }
                    }
                }

                Text {
                    text: qsTr("Configuration values (see below for details):")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    Layout.topMargin: 4
                }

                RowLayout {
                    spacing: 12
                    Layout.fillWidth: true

                    GButton {
                        text: qsTr("📖 View Configuration")
                        type: 2
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 32
                        onClicked: {
                            // 滚动到配置助手卡片（在主页面中实现）
                            ToastManager.ShowInfo(qsTr("See Configuration Helper below"), 2000)
                        }
                    }
                }
            }
        }
    }

    // 步骤区域组件
    component StepSection: ColumnLayout {
        property int stepNumber: 1
        property string stepTitle: ""
        property string stepDescription: ""
        property alias stepContent: contentContainer.data

        spacing: 12

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            // 步骤编号圆圈
            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignTop
                radius: 16
                color: GTheme.primaryColor

                Text {
                    anchors.centerIn: parent
                    text: stepNumber
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "white"
                }
            }

            // 步骤内容
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: stepTitle
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }

                Text {
                    text: stepDescription
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                }

                Item {
                    id: contentContainer
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    implicitHeight: childrenRect.height

                    // 让放入的 stepContent 铺满容器宽度:contentContainer 是普通 Item,
                    // 其子布局不会自动继承宽度;若不绑定,内部 Layout.fillWidth 的子项会因
                    // 容器宽度未定义而坍缩到 ~0,导致文字相互重叠。
                    onChildrenChanged: bindChildWidths()
                    Component.onCompleted: bindChildWidths()
                    function bindChildWidths() {
                        for (var i = 0; i < children.length; i++) {
                            children[i].width = Qt.binding(function () { return contentContainer.width })
                        }
                    }
                }
            }
        }

        // 步骤之间的分隔线
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 8
            color: GTheme.borderLight
        }
    }

    // 浏览器按钮组件
    component BrowserButton: Rectangle {
        property string browserName: ""
        property string iconSource: ""
        signal clicked()

        implicitWidth: 100
        implicitHeight: 100
        radius: 8
        color: hovered ? GTheme.fillLighter : "transparent"
        border.width: 1
        border.color: GTheme.borderBase

        property bool hovered: hoverHandler.hovered

        HoverHandler {
            id: hoverHandler
        }

        Behavior on color {
            ColorAnimation { duration: 200 }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 8

            Image {
                source: iconSource
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                Layout.alignment: Qt.AlignHCenter
                fillMode: Image.PreserveAspectFit

                scale: hovered ? 1.1 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }

            Text {
                text: browserName
                font.pixelSize: 13
                font.weight: Font.Medium
                color: GTheme.textPrimary
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("Download")
                font.pixelSize: 11
                color: GTheme.primaryColor
                Layout.alignment: Qt.AlignHCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    // 安装说明条目组件
    component InstallInstructionItem: Rectangle {
        property string browserIcon: ""
        property string browserName: ""
        property var instructions: []

        Layout.fillWidth: true
        implicitHeight: instrLayout.implicitHeight + 24
        color: GTheme.bgBase
        radius: 6

        ColumnLayout {
            id: instrLayout
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // 浏览器名称
            RowLayout {
                spacing: 8

                Text {
                    text: browserIcon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 16
                    color: GTheme.primaryColor
                }

                Text {
                    text: browserName
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }
            }

            // 指令列表
            Repeater {
                model: instructions

                Text {
                    text: modelData
                    font.pixelSize: 12
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    leftPadding: 20
                }
            }
        }
    }
}
