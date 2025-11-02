import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// Element Plus 风格基础设置页面
Rectangle {
    id: basicSetting
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
        contentHeight: columnLayout.implicitHeight + 60  // 增加底部间距

        ColumnLayout {
            id: columnLayout
            width: scrollView.availableWidth
            spacing: basicSetting.cardSpacing
            anchors.margins: 0
            anchors.topMargin: basicSetting.standardSpacing  // 顶部间距

            // 主题和语言设置卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                Layout.leftMargin: basicSetting.contentMargin
                Layout.rightMargin: basicSetting.contentMargin
                outlined: true
                padding: basicSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: basicSetting.standardSpacing
                    spacing: basicSetting.standardSpacing

                    // 卡片标题
                    Text {
                        text: qsTr("Appearance & Language")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    ThemeSwitch {
                        id: themeSwitch
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                    }

                    RowLayout {
                        id: language
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: basicSetting.standardSpacing
                        Layout.preferredHeight: 100
                        Label {
                            text: qsTr("Language")
                            Layout.preferredWidth: 100
                            font.pixelSize: 14
                            color: GTheme.textPrimary
                            verticalAlignment: Text.AlignVCenter
                        }

                        GComBoBox {
                            id: languageComBoBox
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 36
                            property var values: LanguageManager.GetSupportedLanguages()
                            model: ["English","简体中文","繁體中文","日本語","한국어"]
                            onActivated: function(selectIndex){
                                let index = selectIndex
                                let value = languageComBoBox.values[index]
                                LanguageManager.SwitchLanguage(value)
                            }
                            Component.onCompleted: {
                                let index = languageComBoBox.values.indexOf(LanguageManager.GetCurrentLanguage())
                                languageComBoBox.currentIndex = index
                            }
                        }
                    }
                }
            }

            // 应用行为设置卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                Layout.leftMargin: basicSetting.contentMargin
                Layout.rightMargin: basicSetting.contentMargin
                outlined: true
                padding: basicSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: basicSetting.standardSpacing
                    spacing: 8

                    // 卡片标题
                    Text {
                        text: qsTr("Application Behavior")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                        Layout.bottomMargin: 8
                    }

                    // 开启自动更新
                    GButtonSwitch {
                        id: autoUpdate
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Auto Update")
                        checked: SettingsManager.qEnableAutoUpdate
                        onClicked: {
                            SettingsManager.SetEnableAutoUpdate(checked)
                        }
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 开机自启动
                    GButtonSwitch {
                        id: autoStart
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Open at Login")
                        checked: SettingsManager.qAutoStart
                        onClicked: {
                            SettingsManager.SetAutoStart(checked)
                            UtilsToolsManager.SetAutoStart(checked)
                        }
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 下次启动记住窗口位置
                    GButtonSwitch {
                        id: rememberWindowPosition
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Remember Window Position")
                        checked: SettingsManager.qRememberWindowPosition
                        onClicked: {
                            SettingsManager.SetRememberWindowPosition(checked)
                        }
                    }

                    Divider {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                    }

                    // 下次启动自动恢复未完成的下载任务
                    GButtonSwitch {
                        id: autoResume
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Auto Resume Unfinished Download")
                        checked: SettingsManager.qAutoResumeTask
                        onClicked: {
                            SettingsManager.SetAria2AutoResumeTask(checked)
                        }
                    }
                }
            }

            // 下载路径设置卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                Layout.leftMargin: basicSetting.contentMargin
                Layout.rightMargin: basicSetting.contentMargin
                outlined: true
                padding: basicSetting.standardSpacing

                RowLayout {
                    id: downloadPath
                    anchors.fill: parent
                    anchors.margins: basicSetting.standardSpacing
                    spacing: basicSetting.standardSpacing

                    // 标签区域
                    ColumnLayout {
                        Layout.preferredWidth: 140
                        spacing: 4

                        Text {
                            text: qsTr("Download Path")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: GTheme.textPrimary
                        }

                        Text {
                            text: qsTr("Global download folder")
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                        }
                    }

                    FolderSelector {
                        id: folderSelector
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        path: SettingsManager.qDir
                        onActived: function(){
                            SettingsManager.SetDir(folderSelector.path)
                            folderSelector.path = Qt.binding(function(){return SettingsManager.qDir})
                        }
                    }
                }
            }

            // 网络代理设置卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: enableProxy.checked ? 150 : 100
                Layout.leftMargin: basicSetting.contentMargin
                Layout.rightMargin: basicSetting.contentMargin
                outlined: true
                padding: basicSetting.standardSpacing

                ColumnLayout {
                    id: proxy
                    anchors.fill: parent
                    anchors.margins: basicSetting.standardSpacing
                    spacing: 12

                    // 卡片标题
                    Text {
                        text: qsTr("Network Proxy")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: GTheme.textPrimary
                        Layout.fillWidth: true
                    }

                    GButtonSwitch {
                        id: enableProxy
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: qsTr("Enable Global Proxy")
                        checked: SettingsManager.qEnableGlobalProxy
                        onClicked: {
                            SettingsManager.SetEnableGlobalProxy(checked)
                            if(!checked){
                                SettingsManager.SetAria2GlobalProxy("")
                            }
                        }
                    }

                    // 代理设置输入区域
                    ColumnLayout {
                        visible: enableProxy.checked
                        Layout.fillWidth: true
                        spacing: 8

                        Divider {
                            Layout.fillWidth: true
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                        }

                        RowLayout {
                            spacing: basicSetting.standardSpacing
                            Layout.fillWidth: true

                            GTextField {
                                id: proxySetting
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                                placeholderText: "[http://][USER:PASSWORD@]HOST[:PORT]"
                                text: SettingsManager.qGlobalProxy
                            }

                            GButton {
                                id: saveProxy
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 36
                                type: 1
                                text: qsTr("Save")
                                onClicked: {
                                    SettingsManager.SetAria2GlobalProxy(proxySetting.text)
                                }
                            }
                        }
                    }
                }
            }
            // 剪贴板监听设置卡片
            GCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 120  // 增加高度以适应内容
                Layout.leftMargin: basicSetting.contentMargin
                Layout.rightMargin: basicSetting.contentMargin
                Layout.bottomMargin: basicSetting.standardSpacing  // 添加底部间距
                outlined: true
                padding: basicSetting.standardSpacing

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: basicSetting.standardSpacing
                    spacing: 8

                    // 卡片标题和描述
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: qsTr("Clipboard Monitor")
                            font.pixelSize: 16
                            font.weight: Font.Medium
                            color: GTheme.textPrimary
                        }

                        Text {
                            text: qsTr("Automatically detect download links from clipboard")
                            font.pixelSize: 12
                            color: GTheme.textSecondary
                        }
                    }

                    GButtonSwitch {
                        id: listenClipboard
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
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
}
