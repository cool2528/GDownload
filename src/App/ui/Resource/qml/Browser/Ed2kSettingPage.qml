import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格 eD2k 设置卡片(暂存-保存派)
// 身份与网络/网络接入/性能三组:本地暂存控件,Save 时校验端口冲突并比对旧值后批量提交
// 骨架走 SettingCard;字段行走 SettingRow;勾选框复合控件保留令牌化布局;
// 操作区走 SettingFormActions;说明提示走 AlertTip
// 分享目录不在此页管理(中心页分享 tab 已覆盖)
// 颜色/尺寸/间距/字号/圆角/动效一律取自 GTheme 令牌;inputWidth 为页面级布局常量
SettingCard {
    id: ed2kSettingPage
    objectName: "ed2kSettingPage"
    Layout.fillWidth: true

    title: qsTr("eD2k Settings")
    description: qsTr("Configure eD2k network identity, connectivity, and performance")

    // 页面级布局常量(非设计令牌,spec 2.2/2.3 约定):端口/并发数输入框宽度
    readonly property int inputWidth: 150

    // ========== 身份与网络 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Identity & Network")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Nickname:")
            control: GTextField {
                id: nicknameField
                objectName: "ed2kNicknameField"
                Layout.preferredWidth: ed2kSettingPage.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qEd2kNickname
                placeholderText: qsTr("GDownload")
                Accessible.name: qsTr("eD2k nickname")
            }
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("TCP Port:")
            hint: qsTr("Takes effect after restarting the app")
            control: GSpinBox {
                id: tcpPortSpinBox
                objectName: "ed2kTcpPortSpinBox"
                Layout.preferredWidth: ed2kSettingPage.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 1024
                to: 65535
                value: SettingsManager.qEd2kTcpPort
                Accessible.name: qsTr("eD2k TCP port")
            }
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("UDP Port:")
            hint: qsTr("Takes effect after restarting the app")
            control: GSpinBox {
                id: udpPortSpinBox
                objectName: "ed2kUdpPortSpinBox"
                Layout.preferredWidth: ed2kSettingPage.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 1024
                to: 65535
                value: SettingsManager.qEd2kUdpPort
                Accessible.name: qsTr("eD2k UDP port")
            }
        }

        // 勾选框 + 标题/说明纵向堆叠:标题较长不适合 SettingRow 的 label 槽,保留令牌化布局
        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            GCheckBox {
                id: obfuscationCheckBox
                objectName: "ed2kObfuscationCheckBox"
                checked: SettingsManager.qEd2kEnableObfuscation
                Accessible.name: qsTr("Enable protocol obfuscation")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Enable Protocol Obfuscation")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Obscures eD2k traffic to help bypass ISP throttling. Takes effect after restarting the app.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        color: GTheme.borderLight
    }

    // ========== 网络接入 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Network Access")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            GCheckBox {
                id: enableKadCheckBox
                objectName: "ed2kEnableKadCheckBox"
                checked: SettingsManager.qEd2kEnableKad
                Accessible.name: qsTr("Enable Kad network")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Enable Kad Network")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Serverless peer discovery via Kademlia DHT. Improves source availability. Takes effect after restarting the app.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            GCheckBox {
                id: autoConnectCheckBox
                objectName: "ed2kAutoConnectCheckBox"
                checked: SettingsManager.qEd2kAutoConnect
                Accessible.name: qsTr("Auto connect on startup")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Auto Connect on Startup")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Automatically connect to the best available server when the app starts.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("server.met URL:")
            hint: qsTr("Used by 'Update from URL' on the eD2k Servers page")
            control: GTextField {
                id: serverMetUrlField
                objectName: "ed2kServerMetUrlField"
                // server.met URL 为长文本，照 Aria2RpcSettingPage 密钥字段做法用 fillWidth 展开容纳
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qEd2kServerMetUrl
                placeholderText: qsTr("http://upd.emule-security.org/server.met")
                Accessible.name: qsTr("eD2k server.met URL")
            }
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Kad nodes source (nodes.dat URL):")
            hint: qsTr("Used to bootstrap the Kad network. Takes effect after restart.")
            control: GTextField {
                id: nodesDatUrlField
                objectName: "ed2kNodesDatUrlField"
                // nodes.dat URL 同 server.met URL，长文本用 fillWidth 展开容纳
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeDefault
                text: SettingsManager.qEd2kNodesDatUrl
                placeholderText: qsTr("http://upd.emule-security.org/nodes.dat")
                Accessible.name: qsTr("eD2k nodes.dat URL")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            GCheckBox {
                id: autoSyncCheckBox
                objectName: "ed2kAutoSyncCheckBox"
                checked: SettingsManager.qEd2kAutoSyncSources
                Accessible.name: qsTr("Auto-sync sources on startup")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Auto-sync sources on startup")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightMedium
                    color: GTheme.textPrimary
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Update the server list and Kad nodes automatically each time the app starts.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // 分隔线:统一 Divider(色 borderLight,spec 3)
    Divider {
        Layout.fillWidth: true
        color: GTheme.borderLight
    }

    // ========== 性能 ==========
    ColumnLayout {
        Layout.fillWidth: true
        spacing: GTheme.spaceMD

        Text {
            Layout.fillWidth: true
            text: qsTr("Performance")
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightMedium
            color: GTheme.textPrimary
        }

        SettingRow {
            Layout.fillWidth: true
            label: qsTr("Max Concurrent Tasks:")
            hint: qsTr("Maximum number of eD2k downloads running at the same time (1-20). Takes effect after restarting the app.")
            control: GSpinBox {
                id: maxConcurrentTasksSpinBox
                objectName: "ed2kMaxConcurrentTasksSpinBox"
                Layout.preferredWidth: ed2kSettingPage.inputWidth
                Layout.preferredHeight: GTheme.sizeDefault
                from: 1
                to: 20
                value: SettingsManager.qEd2kMaxConcurrentTasks
                Accessible.name: qsTr("Maximum concurrent eD2k tasks")
            }
        }
    }

    // ========== 操作区:Reset + Save + 状态提示(暂存-保存派,spec 2.3)==========
    SettingFormActions {
        id: formActions
        Layout.fillWidth: true
        Layout.topMargin: GTheme.spaceLG

        // 有任一控件偏离已保存值时才启用 Save
        hasChanges: nicknameField.text !== SettingsManager.qEd2kNickname ||
                    tcpPortSpinBox.value !== SettingsManager.qEd2kTcpPort ||
                    udpPortSpinBox.value !== SettingsManager.qEd2kUdpPort ||
                    obfuscationCheckBox.checked !== SettingsManager.qEd2kEnableObfuscation ||
                    enableKadCheckBox.checked !== SettingsManager.qEd2kEnableKad ||
                    autoConnectCheckBox.checked !== SettingsManager.qEd2kAutoConnect ||
                    serverMetUrlField.text !== SettingsManager.qEd2kServerMetUrl ||
                    nodesDatUrlField.text !== SettingsManager.qEd2kNodesDatUrl ||
                    autoSyncCheckBox.checked !== SettingsManager.qEd2kAutoSyncSources ||
                    maxConcurrentTasksSpinBox.value !== SettingsManager.qEd2kMaxConcurrentTasks

        onReset: {
            // 重置为默认值(未保存):业务默认值(见 setting.h Default()),非设计令牌
            nicknameField.text = "GDownload"
            tcpPortSpinBox.value = 4662
            udpPortSpinBox.value = 4672
            obfuscationCheckBox.checked = false
            enableKadCheckBox.checked = true
            autoConnectCheckBox.checked = true
            serverMetUrlField.text = "http://upd.emule-security.org/server.met"
            nodesDatUrlField.text = "http://upd.emule-security.org/nodes.dat"
            autoSyncCheckBox.checked = true
            maxConcurrentTasksSpinBox.value = 5

            formActions.statusText = qsTr("Input fields reset to default values (not saved yet)")
            formActions.statusColor = GTheme.textSecondary
        }

        onSave: {
            // 端口冲突校验:TCP/UDP 端口不能相同(照 SpeedControl 页的范围校验先例)
            if (tcpPortSpinBox.value === udpPortSpinBox.value) {
                formActions.statusText = qsTr("TCP and UDP ports must be different")
                formActions.statusColor = GTheme.dangerColor
                ToastManager.ShowError(qsTr("TCP and UDP ports must be different"))
                return
            }

            // 批量应用:值有变才调 setter,并拼接状态摘要(由 SettingFormActions.applySettings 统一收口)
            formActions.applySettings([
                { val: nicknameField.text, old: SettingsManager.qEd2kNickname,
                  setter: function (v) { SettingsManager.SetEd2kNickname(v) },
                  label: qsTr("Nickname") },
                { val: tcpPortSpinBox.value, old: SettingsManager.qEd2kTcpPort,
                  setter: function (v) { SettingsManager.SetEd2kTcpPort(v) },
                  label: qsTr("TcpPort") },
                { val: udpPortSpinBox.value, old: SettingsManager.qEd2kUdpPort,
                  setter: function (v) { SettingsManager.SetEd2kUdpPort(v) },
                  label: qsTr("UdpPort") },
                { val: obfuscationCheckBox.checked, old: SettingsManager.qEd2kEnableObfuscation,
                  setter: function (v) { SettingsManager.SetEd2kEnableObfuscation(v) },
                  label: qsTr("Obfuscation") },
                { val: enableKadCheckBox.checked, old: SettingsManager.qEd2kEnableKad,
                  setter: function (v) { SettingsManager.SetEd2kEnableKad(v) },
                  label: qsTr("Kad") },
                { val: autoConnectCheckBox.checked, old: SettingsManager.qEd2kAutoConnect,
                  setter: function (v) { SettingsManager.SetEd2kAutoConnect(v) },
                  label: qsTr("AutoConnect") },
                { val: serverMetUrlField.text, old: SettingsManager.qEd2kServerMetUrl,
                  setter: function (v) { SettingsManager.SetEd2kServerMetUrl(v) },
                  label: qsTr("ServerMetUrl") },
                { val: nodesDatUrlField.text, old: SettingsManager.qEd2kNodesDatUrl,
                  setter: function (v) { SettingsManager.SetEd2kNodesDatUrl(v) },
                  label: qsTr("NodesDatUrl") },
                { val: autoSyncCheckBox.checked, old: SettingsManager.qEd2kAutoSyncSources,
                  setter: function (v) { SettingsManager.SetEd2kAutoSyncSources(v) },
                  label: qsTr("AutoSyncSources") },
                { val: maxConcurrentTasksSpinBox.value, old: SettingsManager.qEd2kMaxConcurrentTasks,
                  setter: function (v) { SettingsManager.SetEd2kMaxConcurrentTasks(v) },
                  label: qsTr("MaxConcurrentTasks") }
            ], qsTr("eD2k settings saved. Some changes take effect after restart."))
        }
    }

    // 说明提示:分享目录不在此页管理 + 重启生效提醒(告警框→AlertTip)
    AlertTip {
        Layout.fillWidth: true
        severity: "info"
        text: qsTr("Shared folders are managed in the eD2k page. Port, identity and network settings take effect after restarting the app.")
    }
}
