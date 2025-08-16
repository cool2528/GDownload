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
                spacing: 5
             BaiduCookieSettingPage{}
             TrackerServerSettingPage{}
            }
        }
    }
}
