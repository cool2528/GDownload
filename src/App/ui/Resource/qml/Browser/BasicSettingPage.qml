import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
Rectangle{
    id:basicSetting
    color: GTheme.dark ? "#1E1E1E" : GTheme.bgWhite
    clip: true
    ScrollView{
        id:scroView
        anchors.fill: parent
        anchors.margins: 0
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth
        contentHeight: columnLayout.implicitHeight

        ColumnLayout{
            id:columnLayout
            width: scroView.availableWidth
            spacing: 4
            anchors.margins: 0
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 140
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 3
                    Layout.bottomMargin: 3
                    outlined: true
                    padding: 12
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        ThemeSwitch{
                            id:themeSwitch
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                        }
                        RowLayout{
                            id:language
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 10
                            Label{
                                text: qsTr("Language")
                                Layout.preferredWidth: 100
                                font.pixelSize: 14
                                color: GTheme.textPrimary
                                verticalAlignment: Text.AlignVCenter
                            }
                            GComBoBox{
                                id:languageComBoBox
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

                // 基础开关分组
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 3
                    Layout.bottomMargin: 3
                    outlined: true
                    padding: 12
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 5
                        // 开启自动更新
                        GButtonSwitch{
                            id:autoUpdate
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            Layout.margins: 10
                            text: qsTr("Auto Update")
                            checked: SettingsManager.qEnableAutoUpdate
                            onClicked: {
                                SettingsManager.SetEnableAutoUpdate(checked)
                            }
                        }
                        Divider{
                            Layout.fillWidth: true
                            Layout.leftMargin: 5
                            Layout.rightMargin: 5
                        }
                        // 开机自启动
                        GButtonSwitch{
                            id:autoStart
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            Layout.margins: 10
                            text: qsTr("Open at Login")
                            checked: SettingsManager.qAutoStart
                            onClicked: {
                                SettingsManager.SetAutoStart(checked)
                                UtilsToolsManager.SetAutoStart(checked)
                            }
                        }
                        Divider{
                            Layout.fillWidth: true
                            Layout.leftMargin: 5
                            Layout.rightMargin: 5
                        }
                        // 下次启动记住窗口位置
                        GButtonSwitch{
                            id:rememberWindowPosition
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            Layout.margins: 10
                            text: qsTr("Remember Window Position")
                            checked: SettingsManager.qRememberWindowPosition
                            onClicked: {
                                SettingsManager.SetRememberWindowPosition(checked)
                            }
                        }
                        Divider{
                            Layout.fillWidth: true
                            Layout.leftMargin: 5
                            Layout.rightMargin: 5
                        }
                        // 下次启动自动恢复未完成的下载任务
                        GButtonSwitch{
                            id:autoResume
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            Layout.margins: 10
                            text: qsTr("Auto Resume Unfinished Download")
                            checked: SettingsManager.qAutoResumeTask
                            onClicked: {
                                SettingsManager.SetAria2AutoResumeTask(checked)
                            }
                        }
                    }
                }

                // 设置全局下载保存路径
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 3
                    Layout.bottomMargin: 3
                    outlined: true
                    padding: 12
                    RowLayout{
                        id:downloadPath
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10
                        Label{
                            text: qsTr("Download Path")
                            Layout.preferredWidth: 150
                            font.pixelSize: 14
                            color: GTheme.textPrimary
                        }
                        FolderSelector{
                            id:folderSelector
                            Layout.fillWidth: true
                            Layout.rightMargin: 30
                            Layout.preferredHeight: 38
                            path: SettingsManager.qDir
                            onActived: function(){
                                SettingsManager.SetDir(folderSelector.path)
                                folderSelector.path = Qt.binding(function(){return SettingsManager.qDir})
                            }
                        }
                    }
                }

                // 是否启用全局代理
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 130
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 3
                    Layout.bottomMargin: 3
                    outlined: true
                    padding: 12
                    ColumnLayout{
                        id:proxy
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 5
                        GButtonSwitch{
                            id:enableProxy
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            Layout.margins: 10
                            text: qsTr("Enable Global Proxy")
                            checked: SettingsManager.qEnableGlobalProxy
                            onClicked: {
                                SettingsManager.SetEnableGlobalProxy(checked)
                                if(!checked){
                                    SettingsManager.SetAria2GlobalProxy("")
                                }
                            }
                        }
                        Divider{
                            Layout.fillWidth: true
                            Layout.leftMargin: 5
                            Layout.rightMargin: 5
                        }
                        // 代理设置
                        RowLayout {
                            spacing: 10
                            Layout.fillWidth: true
                            Layout.preferredHeight: 45
                            Layout.alignment: Qt.AlignVCenter
                            Layout.margins: 5

                            GTextField {
                                id: proxySetting
                                visible: enableProxy.checked
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                placeholderText: "[http://][USER:PASSWORD@]HOST[:PORT]"
                                text: SettingsManager.qGlobalProxy
                            }

                            // 保存按钮
                            GButton {
                                id: saveProxy
                                visible: enableProxy.checked
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 32
                                Layout.alignment: Qt.AlignVCenter
                                type: 1
                                text: qsTr("Save")
                                onClicked: {
                                    SettingsManager.SetAria2GlobalProxy(proxySetting.text)
                                }
                            }
                        }
                    }
                }
                // 自动监听剪切板链接
                GCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 3
                    Layout.bottomMargin: 3
                    outlined: true
                    padding: 12
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8

                        GButtonSwitch{
                            id:listenClipboard
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

                // 底部间距
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                }
            }
        }
    }
