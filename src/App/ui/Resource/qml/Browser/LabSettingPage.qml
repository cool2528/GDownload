import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./BrowserExtension"
import "../CommonComponents"
import gdl.sdk

// 实验功能页面 - 浏览器插件引导
Rectangle {
    id: labSetting
    color: GTheme.bgPage
    clip: true

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int cardSpacing: 12
    readonly property int contentMargin: 2

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        contentHeight: columnLayout.implicitHeight + 40

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: labSetting.cardSpacing
            anchors.margins: 0
            anchors.topMargin: labSetting.standardSpacing

            // 页面标题和描述
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: labSetting.contentMargin + 16
                Layout.rightMargin: labSetting.contentMargin + 16
                spacing: 8

                RowLayout {
                    spacing: 12

                    Text {
                        text: "\uEA86"  // extension/puzzle icon
                        font.family: "Segoe Fluent Icons"
                        font.pixelSize: 28
                        color: GTheme.primaryColor
                    }

                    Text {
                        text: qsTr("Browser Extension")
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        color: GTheme.textPrimary
                    }
                }

                Text {
                    text: qsTr("Enhance your download experience with our browser extension. Capture links from any webpage and send them directly to GDownload.")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }

                // 分隔线
                Divider {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: 8
                    color: GTheme.borderLight
                }
            }

            // 功能亮点卡片
            FeatureHighlightCard {
                Layout.fillWidth: true
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
            }

            // 安装指南卡片
            InstallationGuideCard {
                Layout.fillWidth: true
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
            }

            // 配置助手卡片
            ConfigHelperCard {
                Layout.fillWidth: true
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
            }

            // FAQ 卡片
            FAQCard {
                Layout.fillWidth: true
                Layout.leftMargin: labSetting.contentMargin
                Layout.rightMargin: labSetting.contentMargin
                Layout.bottomMargin: labSetting.standardSpacing
            }
        }
    }
}
