import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// 功能亮点卡片 - 展示浏览器插件核心功能
GCard {
    id: featureCard
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
                text: qsTr("Feature Highlights")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Discover what makes our browser extension special")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // 功能网格 - 2行3列
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            rowSpacing: 16
            columnSpacing: 16

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
                iconColor: "#FAAD14"  // Element Plus warning color
            }

            // 功能 3: Unified UI
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE790"  // palette icon
                title: qsTr("Unified UI")
                description: qsTr("Perfectly matches GDownload's Element Plus design")
                iconColor: "#722ED1"  // Element Plus purple
            }

            // 功能 4: Secure Connection
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE72E"  // lock icon
                title: qsTr("Secure Connection")
                description: qsTr("Direct WebSocket connection to aria2c via JSON-RPC")
                iconColor: "#52C41A"  // Element Plus success color
            }

            // 功能 5: Cross-Browser
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE774"  // globe icon
                title: qsTr("Cross-Browser")
                description: qsTr("Compatible with Chrome, Firefox, and Edge")
                iconColor: "#13C2C2"  // Element Plus cyan
            }

            // 功能 6: Smart Filtering
            FeatureItem {
                Layout.fillWidth: true
                icon: "\uE71C"  // filter icon
                title: qsTr("Smart Filtering")
                description: qsTr("Filter links by file size, type, and custom rules")
                iconColor: "#EB2F96"  // Element Plus magenta
            }
        }
    }

    // 功能项组件
    component FeatureItem: Rectangle {
        property string icon: ""
        property string title: ""
        property string description: ""
        property color iconColor: GTheme.primaryColor

        implicitHeight: itemLayout.implicitHeight + 24
        color: "transparent"
        radius: 8

        // 悬停效果
        HoverHandler {
            id: hoverHandler
        }

        // 背景色变化
        Rectangle {
            anchors.fill: parent
            radius: 8
            color: hoverHandler.hovered ? GTheme.fillLighter : "transparent"
            opacity: 0.5
            
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        ColumnLayout {
            id: itemLayout
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // 图标
            Text {
                text: icon
                font.family: "Segoe Fluent Icons"
                font.pixelSize: 32
                color: iconColor
                Layout.alignment: Qt.AlignHCenter

                // 悬停时的脉冲动画
                scale: hoverHandler.hovered ? 1.1 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
            }

            // 标题
            Text {
                text: title
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // 描述
            Text {
                text: description
                font.pixelSize: 12
                color: GTheme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
        }
    }
}
