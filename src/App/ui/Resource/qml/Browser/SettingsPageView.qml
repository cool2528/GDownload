import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../CommonComponents"
import gdl.sdk 1.0
Item {
    id:control
    RowLayout{
        id:browserLayout
        anchors.fill: parent
        spacing: 0
        Rectangle{
            id:leftMenuBar
            color: GTheme.dark ? "#282828" :"#f2f3f6"
            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            Text {
                id: title
                text: qsTr("Preferences")
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.top: parent.top
                anchors.topMargin: 30
                font.pixelSize: 14
                color: GTheme.dark ? "#ffffff" :  "#3b3b3b"

            }
            ColumnLayout{
                id:bar
                anchors.top: title.bottom
                anchors.topMargin: 5
                width: parent.width
                property int currentIndex: 0
                property var buttonsArr: [basic,advanced,lab]
                ButtonGroup{
                    id:titleGroup
                    onCheckedButtonChanged: {
                        let index  = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                        bar.currentIndex = index
                        console.debug("index ",index)
                    }
                }
                // Basic
                TttleButton{
                    id:basic
                    checkable: true
                    checked: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.PlayerSettings
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Basic")
                    onClicked: {
                        checked = true
                    }
                }
                //Advanced
                TttleButton{
                    id:advanced
                    checkable: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.KeyboardSettings
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Advanced")
                    onClicked: {
                        checked = true
                    }
                }
                //Lab
                TttleButton{
                    id:lab
                    checkable: true
                    ButtonGroup.group:titleGroup
                    iconSource:SegoeFluentIcons.ExploitProtectionSettings
                    Layout.fillWidth: true
                    Layout.minimumHeight: 40
                    Layout.maximumHeight: 40
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    text: qsTr("Lab")
                    onClicked: {
                        checked = true
                    }

                }
            }
        }

        Rectangle{
            id:browser
            color: GTheme.dark ? "#2e2e2e" : "#ffffff"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 380
            Layout.preferredWidth: 380
            SettingPageTitle{
                id:settingTitle
                type: bar.currentIndex
            }

            StackLayout{
                id:settingsStack
                anchors.top: settingTitle.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                currentIndex: bar.currentIndex

                BasicSettingPage{
                    id:basicSetting

                }

                AdvancedSettingPage{
                    id:advancedSetting
                }

                LabSettingPage{
                    id:labSetting
                }
            }
        }
    }
}
