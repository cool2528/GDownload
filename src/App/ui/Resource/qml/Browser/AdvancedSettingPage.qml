import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
import "../Utils/utils.js" as Utils

// Element Plus 风格高级设置页面
Rectangle {
    id: advancedSetting
    color: GTheme.bgPage  // 使用 Element Plus 页面背景色
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
        contentHeight: columnLayout.implicitHeight + 40  // 底部间距

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: advancedSetting.cardSpacing
            anchors.margins: 0
            anchors.topMargin: advancedSetting.standardSpacing  // 顶部间距

            // 速度控制设置卡片
            SpeedControlSettingPage {
                Layout.fillWidth: true
                Layout.leftMargin: advancedSetting.contentMargin
                Layout.rightMargin: advancedSetting.contentMargin
            }

            // Aria2 RPC 设置卡片
            Aria2RpcSettingPage {
                Layout.fillWidth: true
                Layout.leftMargin: advancedSetting.contentMargin
                Layout.rightMargin: advancedSetting.contentMargin
            }

            // 百度网盘设置卡片
            BaiduCookieSettingPage {
                Layout.fillWidth: true
                Layout.leftMargin: advancedSetting.contentMargin
                Layout.rightMargin: advancedSetting.contentMargin
            }

            // Tracker 服务器设置卡片
            TrackerServerSettingPage {
                Layout.fillWidth: true
                Layout.leftMargin: advancedSetting.contentMargin
                Layout.rightMargin: advancedSetting.contentMargin
                Layout.bottomMargin: advancedSetting.standardSpacing  // 底部间距
            }
        }
    }
}
