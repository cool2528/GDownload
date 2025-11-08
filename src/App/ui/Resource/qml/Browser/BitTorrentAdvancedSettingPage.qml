import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 BitTorrent 高级设置卡片
GCard {
    id: btAdvancedSettingPage
    Layout.fillWidth: true
    Layout.preferredHeight: contentLayout.implicitHeight + 48
    outlined: true
    padding: 16  // Element Plus 标准内边距

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 卡片标题和描述
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: qsTr("BitTorrent Advanced Options")
                font.pixelSize: 16
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            Text {
                text: qsTr("Configure DHT, peer connections, and encryption for BitTorrent downloads")
                font.pixelSize: 12
                color: GTheme.textSecondary
            }
        }

        // DHT 启用
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            GCheckBox {
                id: enableDhtCheckBox
                checked: SettingsManager.qEnableDht
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("Enable DHT (Distributed Hash Table)")
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }

                Text {
                    text: qsTr("Helps find more peers for torrent downloads. Also enables UDP tracker support.")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderBase
        }

        // BT 最大对等节点数
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: qsTr("Max Peers Per Torrent")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                GSpinBox {
                    id: btMaxPeersSpinBox
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40
                    from: 0
                    to: 999
                    value: SettingsManager.qBtMaxPeers
                }

                Text {
                    text: qsTr("peers")
                    font.pixelSize: 13
                    color: GTheme.textSecondary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Maximum number of peers to connect per torrent (0 = unlimited)")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderBase
        }

        // BT 要求加密
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            GCheckBox {
                id: btRequireCryptoCheckBox
                checked: SettingsManager.qBtRequireCrypto
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("Require Encrypted Connections")
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: GTheme.textPrimary
                }

                Text {
                    text: qsTr("Only accept encrypted BitTorrent handshake. Rejects legacy unencrypted connections.")
                    font.pixelSize: 12
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // 统一操作按钮区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 16

            Item {
                Layout.fillWidth: true
            }

            GButton {
                text: qsTr("Reset to Default")
                type: 3  // Warning 样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    enableDhtCheckBox.checked = true
                    btMaxPeersSpinBox.value = 55
                    btRequireCryptoCheckBox.checked = false

                    statusText.text = qsTr("Input fields reset to default values (not saved yet)")
                    statusText.color = GTheme.textSecondary
                }
            }

            GButton {
                text: qsTr("Save Settings")
                type: 1  // 主要按钮样式
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                onClicked: {
                    // 获取输入值
                    let enableDht = enableDhtCheckBox.checked
                    let btMaxPeers = btMaxPeersSpinBox.value
                    let btRequireCrypto = btRequireCryptoCheckBox.checked

                    // 检查是否有更改
                    let originalEnableDht = SettingsManager.qEnableDht
                    let originalBtMaxPeers = SettingsManager.qBtMaxPeers
                    let originalBtRequireCrypto = SettingsManager.qBtRequireCrypto

                    let hasChanges = (enableDht !== originalEnableDht) ||
                                   (btMaxPeers !== originalBtMaxPeers) ||
                                   (btRequireCrypto !== originalBtRequireCrypto)

                    if (hasChanges) {
                        // 保存设置（会自动通过 RPC 更新 aria2c）
                        SettingsManager.SetAria2EnableDht(enableDht)
                        SettingsManager.SetAria2BtMaxPeers(btMaxPeers)
                        SettingsManager.SetAria2BtRequireCrypto(btRequireCrypto)

                        ToastManager.ShowSuccess(
                            qsTr("✓ BitTorrent advanced settings saved and applied successfully!"),
                            3000
                        )

                        statusText.text = qsTr("✓ Settings saved: DHT=%1, MaxPeers=%2, RequireCrypto=%3")
                            .arg(enableDht ? "Enabled" : "Disabled")
                            .arg(btMaxPeers)
                            .arg(btRequireCrypto ? "Yes" : "No")
                        statusText.color = GTheme.successColor
                    } else {
                        ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
                        statusText.text = qsTr("Settings unchanged.")
                        statusText.color = GTheme.textSecondary
                    }
                }
            }
        }

        // 状态提示文本
        Text {
            id: statusText
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: qsTr("Modify the values above and click 'Save Settings' to apply.")
            font.pixelSize: 12
            color: GTheme.textSecondary
            wrapMode: Text.WordWrap
        }

        // 警告提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: warningText.implicitHeight + 24
            color: GTheme.bgWarning
            radius: 4
            border.color: GTheme.borderWarning
            border.width: 1
            visible: btRequireCryptoCheckBox.checked

            Text {
                id: warningText
                anchors.fill: parent
                anchors.margins: 12
                text: qsTr("⚠️ Warning: Requiring encryption may reduce the number of available peers")
                font.pixelSize: 12
                color: GTheme.textWarning
                wrapMode: Text.WordWrap
            }
        }

        // 说明提示
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: tipText.implicitHeight + 24
            color: GTheme.bgInfo
            radius: 4
            border.color: GTheme.borderInfo
            border.width: 1

            Text {
                id: tipText
                anchors.fill: parent
                anchors.margins: 12
                text: qsTr("💡 Tip: For private torrents, DHT is automatically disabled regardless of this setting")
                font.pixelSize: 12
                color: GTheme.textInfo
                wrapMode: Text.WordWrap
            }
        }
    }
}
