import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import gdl.sdk
Item {
    id:control
    implicitHeight: Math.max(label.implicitHeight + 20, row.implicitHeight + 20)
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
        color: GTheme.textPrimary
    }
    RowLayout{
        id:row
        anchors.left: label.right
        anchors.right: parent.right
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 15
        ThemeButton{
            id:systemThemeButton
            Layout.preferredWidth: 60
            Layout.preferredHeight: 60
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
            Layout.preferredWidth: 60
            Layout.preferredHeight: 60
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
            Layout.preferredWidth: 60
            Layout.preferredHeight: 60
            tipText: qsTr("Dark")
            imageSource: "/images/theme/theme-dark@2x.png"
            ButtonGroup.group: themeButtonGroup
            onClicked: {
                GTheme.Settheme(GThemeType.ThemeMode.kDark)
            }
        }
    }

    Component.onCompleted: {
        let checkedButtonIndex = GTheme.theme
        if (checkedButtonIndex === GThemeType.ThemeMode.kSystem) {
            systemThemeButton.checked = true
        } else if (checkedButtonIndex === GThemeType.ThemeMode.kLight) {
            lightThemeButton.checked = true
        } else if (checkedButtonIndex === GThemeType.ThemeMode.kDark) {
            darkThemeButton.checked = true
        }
    }
}
