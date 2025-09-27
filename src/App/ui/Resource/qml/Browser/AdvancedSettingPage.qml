import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
import "../Utils/utils.js" as Utils
Rectangle{
    id:advancedSetting
    color: GTheme.bgWhite
    clip: true
    ScrollView{
        id: scroView
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        ColumnLayout{
            id:columnLayout
            width: scroView.availableWidth
            spacing: 5
            BaiduCookieSettingPage{}
            TrackerServerSettingPage{}
        }
    }
}
