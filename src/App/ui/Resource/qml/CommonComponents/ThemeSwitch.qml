import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Aurora theme picker: copy stays above a responsive, bounded preview grid.
Item {
    id: control
    objectName: "themeSwitch"

    implicitWidth: 560
    implicitHeight: contentLayout.implicitHeight
    Layout.minimumHeight: implicitHeight

    function syncSelection() {
        const mode = GTheme.theme
        systemThemeButton.checked = mode === GThemeType.ThemeMode.kSystem
        lightThemeButton.checked = mode === GThemeType.ThemeMode.kLight
        darkThemeButton.checked = mode === GThemeType.ThemeMode.kDark
    }

    ButtonGroup {
        id: themeButtonGroup
        exclusive: true
    }

    ColumnLayout {
        id: contentLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: GTheme.spaceMD

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Theme mode")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Choose the appearance that fits your workspace.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        GridLayout {
            id: previewGrid
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            columns: width >= 400 ? 3 : 1
            columnSpacing: GTheme.spaceSM
            rowSpacing: GTheme.spaceSM

            ThemeButton {
                id: systemThemeButton
                objectName: "systemThemeButton"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: implicitHeight
                tipText: qsTr("System")
                description: qsTr("Follow Windows")
                previewMode: "system"
                checkable: true
                ButtonGroup.group: themeButtonGroup
                onClicked: {
                    checked = true
                    GTheme.Settheme(GThemeType.ThemeMode.kSystem)
                }
            }

            ThemeButton {
                id: lightThemeButton
                objectName: "lightThemeButton"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: implicitHeight
                tipText: qsTr("Light")
                description: qsTr("Bright surfaces")
                previewMode: "light"
                checkable: true
                ButtonGroup.group: themeButtonGroup
                onClicked: {
                    checked = true
                    GTheme.Settheme(GThemeType.ThemeMode.kLight)
                }
            }

            ThemeButton {
                id: darkThemeButton
                objectName: "darkThemeButton"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: implicitHeight
                tipText: qsTr("Dark")
                description: qsTr("Low-light surfaces")
                previewMode: "dark"
                checkable: true
                ButtonGroup.group: themeButtonGroup
                onClicked: {
                    checked = true
                    GTheme.Settheme(GThemeType.ThemeMode.kDark)
                }
            }
        }
    }

    Connections {
        target: GTheme
        function onThemeChanged() { control.syncSelection() }
    }

    Component.onCompleted: syncSelection()
}
