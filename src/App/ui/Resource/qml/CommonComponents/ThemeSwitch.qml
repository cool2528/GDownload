import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import gdl.sdk
Item {
    id:control
    ButtonGroup{
        id:themeButtonGroup
    }
    Text {
        id: label
        text: qsTr("Theme Mode:")
        font.bold: true
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 14
        color: GTheme.dark ? "#ffffff" : "#3b3b3b"
    }
    RowLayout{
        id:row
        anchors{
            left: label.right
            leftMargin: 10
            top: parent.top
        }
        spacing: 10
        ThemeButton{
            id:systemThemeButton
            Layout.preferredWidth: 70
            Layout.preferredHeight: 70
            tipText: qsTr("System")
            checkable: true
            imageSource: "/images/theme/theme-auto@2x.png"
            ButtonGroup.group: themeButtonGroup
            onClicked: {
                GTheme.Settheme(GThemeType.ThemeMode.kSystem)
            }
        }

        ThemeButton{
            id:lightThemeButton
            checkable: true
            Layout.preferredWidth: 70
            Layout.preferredHeight: 70
            imageSource: "/images/theme/theme-light@2x.png"
            tipText: qsTr("Light")
            ButtonGroup.group: themeButtonGroup
            onClicked: {
                GTheme.Settheme(GThemeType.ThemeMode.kLight)
            }
        }

        ThemeButton{
            id:darkThemeButton
            checkable: true
            Layout.preferredWidth: 70
            Layout.preferredHeight: 70
            tipText: qsTr("Dark")
            imageSource: "/images/theme/theme-dark@2x.png"
            ButtonGroup.group: themeButtonGroup
            onClicked: {
                GTheme.Settheme(GThemeType.ThemeMode.kDark)
            }
        }
    }

}
