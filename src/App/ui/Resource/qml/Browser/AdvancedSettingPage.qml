import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Element Plus 风格高级设置页面(A 类纯容器)
// 仅作容器嵌入 9 个 B 类设置卡片,自身不承载设置项;
// 颜色/尺寸/间距一律取自 GTheme 令牌,零魔法数字(原本地三常量已删除)
Rectangle {
    id: advancedSetting
    color: GTheme.bgPage
    clip: true

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        contentHeight: columnLayout.implicitHeight + GTheme.space2XL  // 底部留白

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: GTheme.spaceMD  // 卡片间距(原本地常量已令牌化)

            // 速度控制设置卡片
            SpeedControlSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.topMargin: GTheme.spaceLG  // 顶部留白
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 连接和性能参数设置卡片
            ConnectionPerformanceSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 下载完成后操作设置卡片
            PostDownloadActionsSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 超时和重试设置卡片
            TimeoutRetrySettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // BitTorrent 高级设置卡片
            BitTorrentAdvancedSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // User-Agent 设置卡片
            UserAgentSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // Aria2 RPC 设置卡片
            Aria2RpcSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // 百度网盘设置卡片
            BaiduCookieSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
            }

            // Tracker 服务器设置卡片
            TrackerServerSettingPage {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
                Layout.bottomMargin: GTheme.spaceLG  // 底部留白
            }
        }
    }
}
