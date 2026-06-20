import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 功能亮点卡片 - 展示浏览器插件核心功能
// 颜色/尺寸/间距/字号/圆角/动效一律取自 GTheme 令牌,零魔法数字
// 装饰色(紫/青/粉)按 spec Section 7 保留为局部常量:EP 令牌体系无对应语义色
GCard {
    id: featureCard
    Layout.fillWidth: true
    // 单层边距补偿(上下各 spaceLG):消除原 GCard padding:16 + 内层 margins:16 的双重边距
    Layout.preferredHeight: contentLayout.implicitHeight + 2 * GTheme.spaceLG
    outlined: true
    padding: GTheme.spaceLG

    // ========== 装饰色(spec Section 7)==========
    // EP 令牌体系无紫/青/粉语义对应,保留为局部常量;仅用于功能图标着色,不随深浅色切换
    readonly property color decorationPurple: "#722ED1"
    readonly property color decorationCyan: "#13C2C2"
    readonly property color decorationMagenta: "#EB2F96"

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        // 内层边距收为 0:GCard 已通过 padding=spaceLG 提供单层内边距,避免双重边距
        anchors.margins: 0
        spacing: GTheme.spaceLG

        // 卡片标题
        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                text: qsTr("Feature Highlights")
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Discover what makes our browser extension special")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }
        }

        // 功能网格 - 2行3列
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: GTheme.spaceLG
            columnSpacing: GTheme.spaceLG

            // 功能 1: One-Click Capture
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE71B"  // link icon
                title: qsTr("One-Click Capture")
                description: qsTr("Quickly capture download links from web pages")
                iconColor: GTheme.primaryColor
            }

            // 功能 2: Batch Download
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE945"  // flash icon
                title: qsTr("Batch Download")
                description: qsTr("Select multiple links and send them all at once")
                iconColor: GTheme.warningColor  // 橙:映射到 warning 语义色
            }

            // 功能 3: Unified UI
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE790"  // palette icon
                title: qsTr("Unified UI")
                description: qsTr("Perfectly matches GDownload's Element Plus design")
                iconColor: featureCard.decorationPurple  // 紫:装饰色(无 EP 语义对应)
            }

            // 功能 4: Secure Connection
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE72E"  // lock icon
                title: qsTr("Secure Connection")
                description: qsTr("Direct WebSocket connection to aria2c via JSON-RPC")
                iconColor: GTheme.successColor  // 绿:映射到 success 语义色
            }

            // 功能 5: Cross-Browser
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE774"  // globe icon
                title: qsTr("Cross-Browser")
                description: qsTr("Compatible with Chrome, Firefox, and Edge")
                iconColor: featureCard.decorationCyan  // 青:装饰色(无 EP 语义对应)
            }

            // 功能 6: Smart Filtering
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE71C"  // filter icon
                title: qsTr("Smart Filtering")
                description: qsTr("Filter links by file size, type, and custom rules")
                iconColor: featureCard.decorationMagenta  // 粉:装饰色(无 EP 语义对应)
            }
        }
    }

    // 功能项组件
    component FeatureItem: Rectangle {
        property string icon: ""
        property string title: ""
        property string description: ""
        property color iconColor: GTheme.primaryColor

        // 上下内边距各 spaceMD,补偿 itemLayout 的 anchors.margins
        implicitHeight: itemLayout.implicitHeight + 2 * GTheme.spaceMD
        color: "transparent"
        radius: GTheme.radiusBase

        // 悬停效果
        HoverHandler {
            id: hoverHandler
        }

        // 背景色变化
        Rectangle {
            anchors.fill: parent
            radius: GTheme.radiusBase
            color: hoverHandler.hovered ? GTheme.fillLighter : "transparent"
            opacity: 0.5

            Behavior on color {
                ColorAnimation {
                    duration: GTheme.durationBase
                }
            }
        }

        ColumnLayout {
            id: itemLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            // 图标
            Text {
                text: icon
                font.family: "Segoe Fluent Icons"
                font.pixelSize: 32  // 图标尺寸,保留(非设计令牌)
                color: iconColor
                Layout.alignment: Qt.AlignHCenter

                // 悬停时的脉冲动画
                scale: hoverHandler.hovered ? 1.1 : 1.0
                Behavior on scale {
                    NumberAnimation {
                        duration: GTheme.durationBase
                        easing.type: GTheme.easingStandard
                    }
                }
            }

            // 标题
            Text {
                text: title
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // 描述
            Text {
                text: description
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
        }
    }
}
