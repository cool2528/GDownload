import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 BitTorrent 高级设置卡片(暂存-保存派)
// DHT/最大对等节点数/要求加密三项:本地暂存控件,Save 时比对旧值后批量提交
// 骨架走 SettingCard;对等节点数行走 SettingRow;勾选+说明为复合控件保留令牌化布局;
// 操作区走 SettingFormActions;告警框走 AlertTip(修复幽灵令牌 bgWarning/bgInfo/textWarning/textInfo)
// 颜色/尺寸/间距/字号/圆角/动效一律取自 GTheme 令牌;inputWidth 为页面级布局常量
SettingCard {
    id: btAdvancedSettingPage
    Layout.fillWidth: true

    title: qsTr("BitTorrent Advanced Options")
    description: qsTr("Configure DHT, peer connections, and encryption for BitTorrent downloads")

    // 页面级布局常量(非设计令牌,spec 2.2/2.3 约定):对等节点数输入框宽度
    readonly property int inputWidth: 120

    // ========== DHT 启用 ==========
    // 勾选框 + 标题/说明纵向堆叠:标题较长不适合 SettingRow 的 label 槽,保留令牌化布局
    RowLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        GCheckBox {
            id: enableDhtCheckBox
            checked: SettingsManager.qEnableDht
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                Layout.fillWidth: true
                text: qsTr("Enable DHT (Distributed Hash Table)")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Helps find more peers for torrent downloads. Also enables UDP tracker support.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
            }
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        color: GTheme.borderLight
    }

    // ========== BT 最大对等节点数 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Max Peers Per Torrent")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Peers:")
            control: GSpinBox {
                id: btMaxPeersSpinBox
                Layout.preferredWidth: btAdvancedSettingPage.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 0
                to: 999  // 业务值:对等节点数上界(非设计令牌)
                value: SettingsManager.qBtMaxPeers
            }
            hint: qsTr("Maximum number of peers to connect per torrent (0 = unlimited)")
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        color: GTheme.borderLight
    }

    // ========== BT 要求加密 ==========
    // 勾选框 + 标题/说明纵向堆叠:标题较长不适合 SettingRow 的 label 槽,保留令牌化布局
    RowLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        GCheckBox {
            id: btRequireCryptoCheckBox
            checked: SettingsManager.qBtRequireCrypto
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                Layout.fillWidth: true
                text: qsTr("Require Encrypted Connections")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Only accept encrypted BitTorrent handshake. Rejects legacy unencrypted connections.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
            }
        }
    }

    // ========== 操作区:Reset + Save + 状态提示(暂存-保存派,spec 2.3)==========
    SettingFormActions {
        id: formActions
        Layout.fillWidth: true
        Layout.topMargin: GTheme.spaceLG

        // 有任一控件偏离已保存值时才启用 Save
        hasChanges: enableDhtCheckBox.checked !== SettingsManager.qEnableDht ||
                    btMaxPeersSpinBox.value !== SettingsManager.qBtMaxPeers ||
                    btRequireCryptoCheckBox.checked !== SettingsManager.qBtRequireCrypto

        onReset: {
            // 重置为默认值(未保存):业务默认值,非设计令牌
            enableDhtCheckBox.checked = true
            btMaxPeersSpinBox.value = 55
            btRequireCryptoCheckBox.checked = false

            formActions.statusText = qsTr("Input fields reset to default values (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 批量应用:值有变才调 setter,并拼接状态摘要(由 SettingFormActions.applySettings 统一收口)
            formActions.applySettings([
                { val: enableDhtCheckBox.checked, old: SettingsManager.qEnableDht,
                  setter: function (v) { SettingsManager.SetAria2EnableDht(v) },
                  label: qsTr("DHT") },
                { val: btMaxPeersSpinBox.value, old: SettingsManager.qBtMaxPeers,
                  setter: function (v) { SettingsManager.SetAria2BtMaxPeers(v) },
                  label: qsTr("MaxPeers") },
                { val: btRequireCryptoCheckBox.checked, old: SettingsManager.qBtRequireCrypto,
                  setter: function (v) { SettingsManager.SetAria2BtRequireCrypto(v) },
                  label: qsTr("RequireCrypto") }
            ], qsTr("✓ BitTorrent advanced settings saved and applied successfully!"))
        }
    }

    // 警告提示:启用"要求加密"时显示(告警框→AlertTip,修复幽灵令牌 bgWarning/borderWarning/textWarning)
    AlertTip {
        Layout.fillWidth: true
        visible: btRequireCryptoCheckBox.checked
        severity: "warning"
        text: qsTr("Requiring encryption may reduce the number of available peers")
    }

    // 说明提示:私有种子 DHT 行为(告警框→AlertTip,修复幽灵令牌 bgInfo/borderInfo/textInfo)
    AlertTip {
        Layout.fillWidth: true
        severity: "info"
        text: qsTr("For private torrents, DHT is automatically disabled regardless of this setting")
    }
}
