import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 安装指南卡片 - 分步骤引导用户安装浏览器插件
// 颜色/尺寸/间距/字号/圆角/动效一律取自 GTheme 令牌,零魔法数字
// contentContainer 的 bindChildWidths 宽度绑定逻辑为既有坍缩修复,原样保留不动
GCard {
    id: installCard
    Layout.fillWidth: true
    // 单层边距补偿(上下各 spaceLG):消除原 GCard padding:16 + 内层 margins:16 的双重边距
    Layout.preferredHeight: contentLayout.implicitHeight + 2 * GTheme.spaceLG
    outlined: true
    padding: GTheme.spaceLG

    // ========== 页面级布局常量(EP 令牌无对应值)==========
    // 彩色背景上的白字(不随主题切换,参照 GButton onAccentText 约定)
    readonly property color onAccentText: "#FFFFFF"
    // 浏览器按钮尺寸(正方形,EP 尺寸令牌无对应,页面级布局常量)
    readonly property int browserButtonSize: 100
    // 浏览器图标尺寸(EP 令牌无对应,页面级布局常量)
    readonly property int browserIconSize: 48
    // 配置按钮宽度(EP 令牌无对应,页面级布局常量)
    readonly property int configButtonWidth: 160

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        // 内层边距收为 0:GCard 已通过 padding=spaceLG 提供单层内边距,避免双重边距
        anchors.margins: 0
        spacing: GTheme.spaceXL

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                text: qsTr("Installation Guide")
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Follow these simple steps to get started")
                font.pixelSize: GTheme.fontCaption
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
                spacing: GTheme.spaceMD

                Text {
                    text: qsTr("Select your browser:")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                }

                // 浏览器按钮行
                RowLayout {
                    spacing: GTheme.spaceLG
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

                // 分隔线(色 borderLight)
                Divider {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: GTheme.spaceXS
                    color: GTheme.borderLight
                }

                // 其他下载方式
                RowLayout {
                    spacing: GTheme.spaceXL
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: GTheme.spaceSM

                        Text {
                            text: "\uE943"  // code/github icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: GTheme.fontSubtitle
                            color: GTheme.textSecondary
                        }

                        Text {
                            text: qsTr("GitHub Release:")
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textRegular
                        }

                        Text {
                            text: qsTr("Visit Repository")
                            font.pixelSize: GTheme.fontCaption
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
                        spacing: GTheme.spaceSM

                        Text {
                            text: "\uE774"  // globe icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: GTheme.fontSubtitle
                            color: GTheme.textSecondary
                        }

                        Text {
                            text: qsTr("Official Site:")
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textRegular
                        }

                        Text {
                            text: qsTr("Download from gdownload.uk")
                            font.pixelSize: GTheme.fontCaption
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
                spacing: GTheme.spaceMD

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

                // 提示信息(裸 Rectangle 告警框→AlertTip,info 语义)
                AlertTip {
                    Layout.fillWidth: true
                    severity: "info"
                    text: qsTr("Coming Soon: Direct installation from browser web stores!")
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
                spacing: GTheme.spaceMD

                Text {
                    text: qsTr("The extension needs to connect to GDownload's aria2c:")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                // 配置说明列表
                ColumnLayout {
                    spacing: GTheme.spaceSM
                    Layout.fillWidth: true

                    Row {
                        spacing: GTheme.spaceSM
                        Text {
                            text: "✅"
                            font.pixelSize: GTheme.fontBody
                        }
                        Text {
                            text: qsTr("Default settings are pre-configured")
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textRegular
                        }
                    }

                    Row {
                        spacing: GTheme.spaceSM
                        Text {
                            text: "✅"
                            font.pixelSize: GTheme.fontBody
                        }
                        Text {
                            text: qsTr("Works out-of-the-box with GDownload")
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textRegular
                        }
                    }
                }

                Text {
                    text: qsTr("Configuration values (see below for details):")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    Layout.topMargin: GTheme.spaceXS
                }

                RowLayout {
                    spacing: GTheme.spaceMD
                    Layout.fillWidth: true

                    GButton {
                        text: qsTr("📖 View Configuration")
                        type: 2
                        Layout.preferredWidth: installCard.configButtonWidth
                        Layout.preferredHeight: GTheme.sizeDefault
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

        spacing: GTheme.spaceMD

        RowLayout {
            spacing: GTheme.spaceMD
            Layout.fillWidth: true

            // 步骤编号圆圈
            Rectangle {
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                Layout.alignment: Qt.AlignTop
                radius: GTheme.sizeDefault / 2
                color: GTheme.primaryColor

                Text {
                    anchors.centerIn: parent
                    text: stepNumber
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                    color: installCard.onAccentText
                }
            }

            // 步骤内容
            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceSM

                Text {
                    text: stepTitle
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }

                Text {
                    text: stepDescription
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                }

                Item {
                    id: contentContainer
                    Layout.fillWidth: true
                    Layout.topMargin: GTheme.spaceXS
                    implicitHeight: childrenRect.height

                    // 让放入的 stepContent 铺满容器宽度:contentContainer 是普通 Item,
                    // 其子布局不会自动继承宽度;若不绑定,内部 Layout.fillWidth 的子项会因
                    // 容器宽度未定义而坍缩到 ~0,导致文字相互重叠。
                    // (既有宽度坍缩修复,原样保留,勿动)
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

        // 步骤之间的分隔线(色 borderLight)
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: GTheme.spaceSM
            color: GTheme.borderLight
        }
    }

    // 浏览器按钮组件
    component BrowserButton: Rectangle {
        property string browserName: ""
        property string iconSource: ""
        signal clicked()

        implicitWidth: installCard.browserButtonSize
        implicitHeight: installCard.browserButtonSize
        radius: GTheme.radiusBase
        color: hovered ? GTheme.fillLighter : "transparent"
        border.width: 1
        border.color: GTheme.borderBase

        property bool hovered: hoverHandler.hovered

        HoverHandler {
            id: hoverHandler
        }

        Behavior on color {
            ColorAnimation {
                duration: GTheme.durationBase
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: GTheme.spaceSM

            Image {
                source: iconSource
                Layout.preferredWidth: installCard.browserIconSize
                Layout.preferredHeight: installCard.browserIconSize
                Layout.alignment: Qt.AlignHCenter
                fillMode: Image.PreserveAspectFit

                scale: hovered ? 1.1 : 1.0
                Behavior on scale {
                    NumberAnimation {
                        duration: GTheme.durationBase
                        easing.type: GTheme.easingStandard
                    }
                }
            }

            Text {
                text: browserName
                font.pixelSize: GTheme.fontCaption
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("Download")
                font.pixelSize: GTheme.fontCaption
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
        implicitHeight: instrLayout.implicitHeight + 2 * GTheme.spaceMD
        color: GTheme.bgBase
        radius: GTheme.radiusBase

        ColumnLayout {
            id: instrLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            // 浏览器名称
            RowLayout {
                spacing: GTheme.spaceSM

                Text {
                    text: browserIcon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }

                Text {
                    text: browserName
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }
            }

            // 指令列表
            Repeater {
                model: instructions

                Text {
                    text: modelData
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    leftPadding: GTheme.spaceXL
                }
            }
        }
    }
}
