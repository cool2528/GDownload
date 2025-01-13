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
                spacing: 10
                ThemeSwitch{
                    id:themeSwitch
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.margins: 10
                }
            }
        }
    }
}
