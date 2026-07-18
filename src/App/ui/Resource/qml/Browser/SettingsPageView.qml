import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0

// Aurora preferences shell. The navigation becomes a compact horizontal rail
// before the former fixed sidebar could force the content outside the window.
Item {
    id: control
    objectName: "settingsShell"

    property alias currentIndex: bar.currentIndex
    readonly property bool compactLayout: width < 760
    readonly property int shellSpacing: compactLayout ? GTheme.spaceMD : GTheme.spaceLG
    readonly property int shellRadius: GTheme.radiusLarge
    readonly property int shellSidebarWidth: GTheme.sidebarWidth

    GridLayout {
        id: browserLayout
        anchors.fill: parent
        columns: control.compactLayout ? 1 : 2
        columnSpacing: 0
        rowSpacing: 0

        Rectangle {
            id: leftMenuBar
            objectName: "settingsSidebar"
            Layout.fillWidth: control.compactLayout
            Layout.fillHeight: !control.compactLayout
            Layout.minimumWidth: control.compactLayout ? 0 : control.shellSidebarWidth
            Layout.preferredWidth: control.compactLayout ? -1 : control.shellSidebarWidth
            Layout.maximumWidth: control.compactLayout ? 100000 : control.shellSidebarWidth
            Layout.preferredHeight: control.compactLayout
                                    ? compactSidebarLayout.implicitHeight + control.shellSpacing * 2
                                    : -1
            color: GTheme.surfaceBase

            Rectangle {
                anchors.right: control.compactLayout ? undefined : parent.right
                anchors.bottom: control.compactLayout ? parent.bottom : undefined
                width: control.compactLayout ? parent.width : 1
                height: control.compactLayout ? 1 : parent.height
                color: GTheme.borderLight
            }

            ColumnLayout {
                id: compactSidebarLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: control.shellSpacing
                spacing: GTheme.spaceMD

                Text {
                    id: title
                    objectName: "settingsTitle"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Preferences")
                    font.pixelSize: control.compactLayout ? GTheme.fontSubtitle : GTheme.fontTitle
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                GridLayout {
                    id: bar
                    objectName: "settingsNav"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    columns: control.compactLayout ? 3 : 1
                    columnSpacing: control.compactLayout ? GTheme.spaceSM : 0
                    rowSpacing: GTheme.spaceXS

                    property int currentIndex: 0
                    property var buttonsArr: [basic, advanced, lab]

                    ButtonGroup {
                        id: titleGroup
                        onCheckedButtonChanged: {
                            const index = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                            if (index >= 0)
                                bar.currentIndex = index
                        }
                    }

                    GButton {
                        id: basic
                        objectName: "settingsBasicNav"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: GTheme.navItemHeight
                        variant: "nav"
                        checkable: true
                        checked: true
                        ButtonGroup.group: titleGroup
                        iconName: "settings"
                        text: qsTr("Basic")
                        activeFocusOnTab: true
                        Accessible.name: text
                        onClicked: checked = true
                    }

                    GButton {
                        id: advanced
                        objectName: "settingsAdvancedNav"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: GTheme.navItemHeight
                        variant: "nav"
                        checkable: true
                        ButtonGroup.group: titleGroup
                        iconName: "keyboard"
                        text: qsTr("Advanced")
                        activeFocusOnTab: true
                        Accessible.name: text
                        onClicked: checked = true
                    }

                    GButton {
                        id: lab
                        objectName: "settingsLabNav"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: GTheme.navItemHeight
                        variant: "nav"
                        checkable: true
                        ButtonGroup.group: titleGroup
                        iconName: "lightbulb"
                        text: qsTr("Lab")
                        activeFocusOnTab: true
                        Accessible.name: text
                        onClicked: checked = true
                    }
                }
            }
        }

        Rectangle {
            id: browser
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            color: GTheme.bgPage

            Rectangle {
                anchors.fill: parent
                anchors.margins: control.shellSpacing
                radius: control.shellRadius
                color: GTheme.surfaceElevated
                border.width: 1
                border.color: GTheme.borderLighter
            }

            SettingPageTitle {
                id: settingTitle
                type: bar.currentIndex
                anchors.top: parent.top
                anchors.topMargin: control.shellSpacing + GTheme.spaceMD
                anchors.left: parent.left
                anchors.leftMargin: control.shellSpacing + GTheme.spaceMD
                anchors.right: parent.right
                anchors.rightMargin: control.shellSpacing + GTheme.spaceMD
            }

            StackLayout {
                id: settingsStack
                objectName: "settingsStack"
                anchors.top: settingTitle.bottom
                anchors.topMargin: GTheme.spaceMD
                anchors.left: parent.left
                anchors.leftMargin: control.shellSpacing + GTheme.spaceMD
                anchors.right: parent.right
                anchors.rightMargin: control.shellSpacing + GTheme.spaceMD
                anchors.bottom: parent.bottom
                anchors.bottomMargin: control.shellSpacing + GTheme.spaceMD
                currentIndex: bar.currentIndex

                BasicSettingPage {
                    id: basicSetting
                }

                AdvancedSettingPage {
                    id: advancedSetting
                }

                LabSettingPage {
                    id: labSetting
                }
            }
        }
    }
}
