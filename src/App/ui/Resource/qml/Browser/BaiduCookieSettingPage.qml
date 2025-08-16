import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
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
