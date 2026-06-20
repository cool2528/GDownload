import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 常见问题卡片 - FAQ 和支持信息
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字
// 图标字形尺寸/链接项尺寸/Toast 时长为本地业务常量,非设计令牌(已注释标注)
GCard {
    id: faqCard
    Layout.fillWidth: true
    // 高度 = 内容隐式高度 + 上下内边距(单层 spaceLG,消除原双重 16 边距)
    Layout.preferredHeight: contentLayout.implicitHeight + GTheme.spaceLG * 2
    outlined: true
    padding: GTheme.spaceLG

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 0
        spacing: GTheme.spaceXL

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            RowLayout {
                spacing: GTheme.spaceSM

                Text {
                    text: "\uE897"  // help-circle icon
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 18  // 图标尺寸,保留(非设计令牌)
                    color: GTheme.primaryColor
                }

                Text {
                    text: qsTr("Frequently Asked Questions")
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }
            }

            Text {
                text: qsTr("Find answers to common questions")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }
        }

        // FAQ 列表
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            // FAQ 1
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Why do I need the browser extension?")
                answer: qsTr("The extension allows you to capture download links directly from web pages and send them to GDownload with a single click. It seamlessly integrates with your browsing experience.")
            }

            // FAQ 2
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Is my data safe?")
                answer: qsTr("Yes! The extension connects locally to aria2c via WebSocket. All communication stays on your computer - no data is sent to external servers. Your privacy is fully protected.")
            }

            // FAQ 3
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Connection failed. What should I do?")
                answer: qsTr("1. Ensure GDownload is running\n2. Check that aria2c is enabled in GDownload settings\n3. Verify the WebSocket URL and RPC Secret match the values shown above\n4. Try restarting both GDownload and your browser")
            }

            // FAQ 4
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Which browsers are supported?")
                answer: qsTr("The extension supports Chrome 110+, Firefox 115+, and Edge 110+. It uses the standard Web Extensions API (Manifest V3) for maximum compatibility.")
            }

            // FAQ 5
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Can I customize what links are captured?")
                answer: qsTr("Yes! The extension has powerful filtering options:\n• Set minimum file size\n• Filter by file type (video, audio, archive, etc.)\n• Add URL blacklist patterns\n• Configure domain whitelist")
            }

            // FAQ 6
            FAQItem {
                Layout.fillWidth: true
                question: qsTr("Does it work with password-protected sites?")
                answer: qsTr("Yes. The extension can optionally send cookies and authorization headers (disabled by default for security). Enable these in the extension's Privacy & Security settings for sites that require login.")
            }
        }

        // 分隔线(色 borderLight)
        Divider {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        // 需要更多帮助
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Text {
                text: qsTr("📖 Need more help?")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            RowLayout {
                spacing: GTheme.spaceXL
                Layout.fillWidth: true

                // GitHub 问题
                HelpLinkItem {
                    icon: "\uEBEF"  // github icon
                    label: qsTr("GitHub Issues")
                    description: qsTr("Report bugs")
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension/issues")
                        ToastManager.ShowInfo(qsTr("Opening GitHub Issues..."), 2000)
                    }
                }

                // 官方文档
                HelpLinkItem {
                    icon: "\uE85D"  // book icon
                    label: qsTr("Documentation")
                    description: qsTr("Full user guide")
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/cool2528/gd-browser-extension/blob/main/README.md")
                        ToastManager.ShowInfo(qsTr("Opening documentation..."), 2000)
                    }
                }

                // 官网
                HelpLinkItem {
                    icon: "\uE86F"  // globe icon
                    label: qsTr("Official Website")
                    description: qsTr("Visit gdownload.uk")
                    onClicked: {
                        Qt.openUrlExternally("https://gdownload.uk/")
                        ToastManager.ShowInfo(qsTr("Opening website..."), 2000)
                    }
                }
            }
        }

        // 社区支持
        Rectangle {
            Layout.fillWidth: true
            // 高度 = 内容隐式高度 + 上下内边距(单层 spaceLG)
            Layout.preferredHeight: communityLayout.implicitHeight + GTheme.spaceLG * 2
            // 主色淡底:浅色用 primaryLight(9),深色回退 fillLight(替代原 Qt.rgba 透明叠加)
            color: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)
            radius: GTheme.radiusBase

            ColumnLayout {
                id: communityLayout
                anchors.fill: parent
                anchors.margins: GTheme.spaceLG
                spacing: GTheme.spaceSM

                RowLayout {
                    spacing: GTheme.spaceSM

                    Text {
                        text: "\uE716"  // people icon
                        font.family: "Segoe Fluent Icons"
                        font.pixelSize: 18  // 图标尺寸,保留(非设计令牌)
                        color: GTheme.primaryColor
                    }

                    Text {
                        text: qsTr("Join Our Community")
                        font.pixelSize: GTheme.fontBody
                        font.weight: GTheme.weightMedium
                        color: GTheme.textPrimary
                    }
                }

                Text {
                    text: qsTr("Get help from other users, share tips, and stay updated with the latest features. Star us on GitHub to show your support!")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }

                GButton {
                    text: qsTr("⭐ Star on GitHub")
                    type: 1
                    Layout.preferredWidth: 140  // 按钮宽度(本地布局常量,非设计令牌)
                    Layout.preferredHeight: GTheme.sizeDefault
                    Layout.topMargin: GTheme.spaceXS
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/cool2528/GDownload")
                        ToastManager.ShowSuccess(qsTr("Thank you for your support!"), 2000)
                    }
                }
            }
        }
    }

    // FAQ 问答项组件
    component FAQItem: Rectangle {
        property string question: ""
        property string answer: ""
        property bool expanded: false
        // 折叠态固定高度(本地布局常量,非设计令牌)
        readonly property int collapsedHeight: 56

        implicitHeight: expanded ? (itemLayout.implicitHeight + GTheme.spaceMD * 2) : collapsedHeight
        color: hoverHandler.hovered ? GTheme.fillLighter : GTheme.bgBase
        radius: GTheme.radiusBase

        Behavior on implicitHeight {
            NumberAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }

        HoverHandler {
            id: hoverHandler
        }

        ColumnLayout {
            id: itemLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            // 问题行
            RowLayout {
                spacing: GTheme.spaceMD
                Layout.fillWidth: true

                // 展开/收起图标
                Text {
                    text: expanded ? "\uE70D" : "\uE76C"  // chevron-down / chevron-right
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 20  // 图标尺寸,保留(非设计令牌)
                    color: GTheme.primaryColor

                    Behavior on rotation {
                        NumberAnimation { duration: GTheme.durationBase }
                    }
                }

                // 问题文本
                Text {
                    text: question
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }

            // 答案（可展开）
            Text {
                text: answer
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.space3XL
                wrapMode: Text.WordWrap
                lineHeight: 1.5
                visible: expanded
                opacity: expanded ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: GTheme.durationBase }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                expanded = !expanded
            }
        }
    }

    // 帮助链接项组件
    component HelpLinkItem: Rectangle {
        property string icon: ""
        property string label: ""
        property string description: ""
        signal clicked()

        implicitWidth: 140  // 链接项宽度(本地布局常量,非设计令牌)
        implicitHeight: 80  // 链接项高度(本地布局常量,非设计令牌)
        radius: GTheme.radiusBase
        color: hovered ? GTheme.fillLight : "transparent"
        border.width: 1
        border.color: GTheme.borderBase

        property bool hovered: hoverHandler.hovered

        HoverHandler {
            id: hoverHandler
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: GTheme.spaceSM

            Text {
                text: icon
                font.family: "Segoe Fluent Icons"
                font.pixelSize: 24  // 图标尺寸,保留(非设计令牌)
                color: GTheme.primaryColor
                Layout.alignment: Qt.AlignHCenter

                scale: hovered ? 1.15 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
                }
            }

            Text {
                text: label
                font.pixelSize: GTheme.fontCaption
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: description
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignHCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
