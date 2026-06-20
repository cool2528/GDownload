import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./BrowserExtension"
import "../CommonComponents"
import gdl.sdk

// 实验功能页面 - 浏览器插件引导(A 类整页容器)
// 颜色/尺寸/间距/字号/动效一律取自 GTheme 令牌,零魔法数字
// 4 个 BrowserExtension 子卡片为独立组件,本页仅做布局编排,不重复卡片骨架
Rectangle {
    id: labSetting
    color: GTheme.bgPage
    clip: true

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        // 底部留白:2 倍大间距 + 小间距,保证内容可完整滚动
        contentHeight: columnLayout.implicitHeight + GTheme.spaceLG * 2 + GTheme.spaceSM

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: GTheme.spaceMD
            anchors.margins: 0
            anchors.topMargin: GTheme.spaceLG

            // 页面标题和描述
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
                spacing: GTheme.spaceSM

                RowLayout {
                    spacing: GTheme.spaceMD

                    Text {
                        text: ""  // extension/puzzle icon
                        font.family: "Segoe Fluent Icons"
                        font.pixelSize: 28  // 图标尺寸,保留(非设计令牌)
                        color: GTheme.primaryColor
                    }

                    Text {
                        text: qsTr("Browser Extension")
                        font.pixelSize: GTheme.fontTitle
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                    }
                }

                Text {
                    text: qsTr("Enhance your download experience with our browser extension. Capture links from any webpage and send them directly to GDownload.")
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textSecondary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }

                // 分隔线(色 borderLight)
                Divider {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    Layout.topMargin: GTheme.spaceSM
                    color: GTheme.borderLight
                }
            }

            // 功能亮点卡片
            FeatureHighlightCard {
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 安装指南卡片
            InstallationGuideCard {
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 配置助手卡片
            ConfigHelperCard {
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // FAQ 卡片
            FAQCard {
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
                Layout.bottomMargin: GTheme.spaceLG
            }
        }
    }
}
