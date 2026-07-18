import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格基础设置页面(A 类整页,即时提交派)
// 卡片骨架统一用 SettingCard(单层 spaceLG 内边距,消除原双重 16 边距);
// 设置项行用 SettingRow;尺寸/间距/字号/圆角全走 GTheme 令牌,零魔法数字。
// 原 A 类本地三常量(standardSpacing/cardSpacing/contentMargin)已删除,改用 GTheme 令牌。
Rectangle {
    id: basicSetting
    color: GTheme.bgPage
    clip: true

    ScrollView {
        id: scrollView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        // 底部留白 spaceLG,与首卡片顶部 spaceLG 边距对称
        contentHeight: columnLayout.implicitHeight + GTheme.spaceLG

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: GTheme.spaceMD

            // ========== 主题和语言设置 ==========
            SettingCard {
                title: qsTr("Appearance & Language")
                description: qsTr("Personalize the app appearance and display language")
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG
                Layout.topMargin: GTheme.spaceLG

                // Aurora 主题预览自绘并响应内容宽度，窄窗口改为单列以避免遮挡。
                ThemeSwitch {
                    id: themeSwitch
                    Layout.fillWidth: true
                }

                // 语言选择(label + control 三段式行)
                SettingRow {
                    Layout.fillWidth: true
                    label: qsTr("Language")
                    control: GComBoBox {
                        id: languageComBoBox
                        Layout.preferredWidth: 150  // 本地布局常量:语言下拉固定宽
                        Layout.preferredHeight: GTheme.sizeDefault
                        Accessible.name: qsTr("Display language")
                        property var values: LanguageManager.GetSupportedLanguages()
                        model: [
                            qsTr("English"),
                            qsTr("Simplified Chinese"),
                            qsTr("Traditional Chinese"),
                            qsTr("Japanese"),
                            qsTr("Korean")
                        ]
                        onActivated: function(selectIndex) {
                            let index = selectIndex
                            let value = languageComBoBox.values[index]
                            LanguageManager.SwitchLanguage(value)
                        }
                        Component.onCompleted: {
                            let index = languageComBoBox.values.indexOf(LanguageManager.GetCurrentLanguage())
                            languageComBoBox.currentIndex = Math.max(0, index)  // indexOf 未命中(-1)时回退第 0 项，杜绝越界(B9)
                        }
                    }
                }
            }

            // ========== 应用行为设置 ==========
            SettingCard {
                title: qsTr("Application Behavior")
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG

                // 开启自动更新
                GButtonSwitch {
                    id: autoUpdate
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Auto Update")
                    checked: SettingsManager.qEnableAutoUpdate
                    onClicked: {
                        SettingsManager.SetEnableAutoUpdate(checked)
                    }
                }

                Divider { Layout.fillWidth: true }

                // 开机自启动
                GButtonSwitch {
                    id: autoStart
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Open at Login")
                    checked: SettingsManager.qAutoStart
                    onClicked: {
                        SettingsManager.SetAutoStart(checked)
                        UtilsToolsManager.SetAutoStart(checked)
                    }
                }

                Divider { Layout.fillWidth: true }

                // 下次启动记住窗口位置
                GButtonSwitch {
                    id: rememberWindowPosition
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Remember Window Position")
                    checked: SettingsManager.qRememberWindowPosition
                    onClicked: {
                        SettingsManager.SetRememberWindowPosition(checked)
                    }
                }

                Divider { Layout.fillWidth: true }

                // 下次启动自动恢复未完成的下载任务
                GButtonSwitch {
                    id: autoResume
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Auto Resume Unfinished Download")
                    checked: SettingsManager.qAutoResumeTask
                    onClicked: {
                        SettingsManager.SetAria2AutoResumeTask(checked)
                    }
                }

                Divider { Layout.fillWidth: true }

                // 关闭时显示确认对话框
                GButtonSwitch {
                    id: showCloseConfirm
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Show Close Confirmation")
                    checked: SettingsManager.qShowCloseConfirm
                    onClicked: {
                        SettingsManager.SetShowCloseConfirm(checked)
                    }
                }
            }

            // ========== 下载路径设置 ==========
            SettingCard {
                title: qsTr("Download Path")
                description: qsTr("Global download folder")
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG

                FolderSelector {
                    id: folderSelector
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    path: SettingsManager.qDir
                    onActived: function() {
                        SettingsManager.SetAria2Dir(folderSelector.path)
                        folderSelector.path = Qt.binding(function() { return SettingsManager.qDir })
                    }
                }
            }

            // ========== 网络代理设置 ==========
            SettingCard {
                title: qsTr("Network Proxy")
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG

                // 启用全局代理
                GButtonSwitch {
                    id: enableProxy
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Enable Global Proxy")
                    checked: SettingsManager.qEnableGlobalProxy
                    onClicked: {
                        SettingsManager.SetEnableGlobalProxy(checked)
                        if (!checked) {
                            SettingsManager.SetAria2GlobalProxy("")
                        }
                    }
                }

                // 代理地址输入区(仅在启用代理时可见;ColumnLayout 自动折叠不可见子项,卡片高度自适应)
                ColumnLayout {
                    visible: enableProxy.checked
                    Layout.fillWidth: true
                    spacing: GTheme.spaceSM

                    Divider { Layout.fillWidth: true }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: GTheme.spaceLG

                        GTextField {
                            id: proxySetting
                            Layout.fillWidth: true
                            Layout.preferredHeight: GTheme.sizeDefault
                            Accessible.name: qsTr("Global proxy address")
                            placeholderText: "[http://][USER:PASSWORD@]HOST[:PORT]"
                            text: SettingsManager.qGlobalProxy
                        }

                        GButton {
                            id: saveProxy
                            Layout.preferredWidth: 80  // 本地布局常量:保存按钮固定宽
                            Layout.preferredHeight: GTheme.sizeDefault
                            type: 1
                            text: qsTr("Save")
                            onClicked: {
                                SettingsManager.SetAria2GlobalProxy(proxySetting.text)
                            }
                        }
                    }
                }
            }

            // ========== 剪贴板监听设置 ==========
            SettingCard {
                title: qsTr("Clipboard Monitor")
                description: qsTr("Automatically detect download links from clipboard")
                Layout.fillWidth: true
                Layout.leftMargin: GTheme.spaceLG
                Layout.rightMargin: GTheme.spaceLG

                GButtonSwitch {
                    id: listenClipboard
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.sizeDefault
                    text: qsTr("Auto Listen Clipboard Link")
                    checked: SettingsManager.qListenClipboard
                    onClicked: {
                        SettingsManager.SetListenClipboard(checked)
                    }
                }
            }
        }
    }
}
