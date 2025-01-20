import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
Rectangle{
    id:basicSetting
    color: GTheme.dark ? "#2e2e2e" :"#ffffff"
    ScrollView{
        id:scroView
        width: parent.width
        height: parent.height
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
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
                       font.pixelSize: 14
                       color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
                   }
                   GComBoBox{
                       id:languageComBoBox
                       Layout.preferredWidth: 140
                       Layout.preferredHeight: 30
                       Layout.margins: 10
                       model: ["English","简体中文","繁體中文","日本語","한국어","русский","Deutsch"]
                   }
               }

                // 开启自动更新
                GButtonSwitch{
                    id:autoUpdate
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Auto Update")
                }

                // 开机自启动
                GButtonSwitch{
                    id:autoStart
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Open at Login")
                }

                // 下次启动记住窗口位置
                GButtonSwitch{
                    id:rememberWindowPosition
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Remember Window Position")
                }

                // 下次启动自动恢复未完成的下载任务
                GButtonSwitch{
                    id:autoResume
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 30
                    Layout.margins: 10
                    text: qsTr("Auto Resume Unfinished Download")
                }

                // 设置全局下载保存路径
                RowLayout{
                    id:downloadPath
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Label{
                        text: qsTr("Download Path")
                        Layout.preferredWidth: 180
                        font.pixelSize: 14
                        color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
                    }
                    FolderSelector{
                        id:folderSelector
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        path: SettingsManager.qDir
                        onActived: function(){
                            SettingsManager.SetDir(folderSelector.path)
                            folderSelector.path = Qt.binding(function(){return SettingsManager.qDir})
                        }
                    }
                }
            }
        }
    }
}
