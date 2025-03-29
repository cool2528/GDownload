import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
Rectangle{
    id:basicSetting
    color: GTheme.dark ? "#2e2e2e" :"#ffffff"
    clip: true
    ScrollView{
        id:scroView
        width: parent.width
        height: parent.height
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        contentItem: Flickable{
            id:flick
            implicitWidth: flick.width
            contentHeight: columnLayout.height
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout{
                id:columnLayout
                width: parent.width
                spacing: 15
                ThemeSwitch{
                    id:themeSwitch
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.margins: 10
                }
                // 设置语言
                RowLayout{
                    id:language
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Label{
                        text: qsTr("Language")
                        Layout.preferredWidth: 180
                        Layout.leftMargin: 5
                        font.pixelSize: 14
                        color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
                    }
                    GComBoBox{
                        id:languageComBoBox
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 30
                        Layout.margins: 10
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

                // 开启自动更新
                GButtonSwitch{
                    id:autoUpdate
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Auto Update")
                    checked: SettingsManager.qEnableAutoUpdate
                    onClicked: {
                        SettingsManager.SetEnableAutoUpdate(checked)
                    }
                }

                // 开机自启动
                GButtonSwitch{
                    id:autoStart
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Open at Login")
                    checked: SettingsManager.qAutoStart
                    onClicked: {
                        SettingsManager.SetAutoStart(checked)
                        UtilsToolsManager.SetAutoStart(checked)
                    }
                }

                // 下次启动记住窗口位置
                GButtonSwitch{
                    id:rememberWindowPosition
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Remember Window Position")
                    checked: SettingsManager.qRememberWindowPosition
                    onClicked: {
                        SettingsManager.SetRememberWindowPosition(checked)
                    }
                }

                // 下次启动自动恢复未完成的下载任务
                GButtonSwitch{
                    id:autoResume
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Auto Resume Unfinished Download")
                    checked: SettingsManager.qAutoResumeTask
                    onClicked: {
                        SettingsManager.SetAria2AutoResumeTask(checked)
                    }
                }

                // 设置全局下载保存路径
                RowLayout{
                    id:downloadPath
                    Layout.fillWidth: true
                    Layout.leftMargin: 15
                    spacing: 0
                    Label{
                        text: qsTr("Download Path")
                        Layout.preferredWidth: 150
                        font.pixelSize: 14
                        color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
                    }
                    FolderSelector{
                        id:folderSelector
                        Layout.fillWidth: true
                        Layout.rightMargin: 30
                        Layout.preferredHeight: 30
                        path: SettingsManager.qDir
                        onActived: function(){
                            SettingsManager.SetDir(folderSelector.path)
                            folderSelector.path = Qt.binding(function(){return SettingsManager.qDir})
                        }
                    }
                }

                // 是否启用全局代理
                ColumnLayout{
                    id:proxy
                    Layout.fillWidth: true
                    Layout.margins: 10
                    GButtonSwitch{
                        id:enableProxy
                        Layout.preferredWidth: 320
                        Layout.preferredHeight: 30
                        text: qsTr("Enable Global Proxy")
                        checked: SettingsManager.qEnableGlobalProxy
                        onClicked: {
                            SettingsManager.SetEnableGlobalProxy(checked)
                            if(!checked){
                                SettingsManager.SetAria2GlobalProxy("")
                            }
                        }
                    }
                    // 代理设置
                    RowLayout{
                        spacing: 10
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        GTextField{
                            id:proxySetting
                            visible: enableProxy.checked
                            Layout.preferredWidth: 320
                            Layout.preferredHeight: 30
                            placeholderText: "[http://][USER:PASSWORD@]HOST[:PORT]"
                            text: SettingsManager.qGlobalProxy

                        }

                        // 保存按钮
                        GButton{
                            id:saveProxy
                            visible: enableProxy.checked
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 30
                            type: 1
                            text: qsTr("Save")
                            onClicked: {
                                SettingsManager.SetAria2GlobalProxy(proxySetting.text)
                            }
                        }
                    }

                }
                // 自动监听剪切板链接
                GButtonSwitch{
                    id:listenClipboard
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
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
