import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
import "../Utils/utils.js" as Utils
Rectangle{
    id:advancedSetting
    color: GTheme.dark ? "#2e2e2e" :"#ffffff"
    clip: true
    ScrollView{
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
                Rectangle{
                    id:baiduCookieManager
                    Layout.margins: 10
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    color: "transparent"
                    border.color: "#409EFF"
                    border.width: 1
                    RowLayout{
                        spacing: 10
                        anchors.fill: parent
                        Label{
                            text: qsTr("Baidu Cookie")
                            color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
                            font.pixelSize: 14
                            Layout.preferredWidth: 100
                            Layout.leftMargin: 10
                        }
                        GTextField{
                            id: baiduCookie
                            Layout.fillWidth: true
                            text: SettingsManager.qBaiduPanCookies
                            placeholderText: qsTr("Please enter Baidu Cookie")
                        }

                        GButton{
                            id: baiduCookieButton
                            type: 1
                            text: qsTr("Save")
                            Layout.preferredWidth: 80
                            Layout.rightMargin: 10
                            onClicked:{
                                if(baiduCookie.text != ""){
                                  // todo 保存cokies
                                    let baidu_cokies = Utils.removeNewline(baiduCookie.text)
                                    SettingsManager.SetBaiduPanCookies(baidu_cokies)
                                    ToastManager.ShowSuccess(qsTr("Save Baidu Cokies Succeed"))
                                }
                            }
                        }
                    }
                }
            }
        }
    }


}
