import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import "../CommonComponents"
import gdl.sdk 1.0

// Element Plus 风格设置页面
Item {
    id: control
    property alias currentIndex: bar.currentIndex

    // Element Plus 设计标准
    readonly property int standardSpacing: 16
    readonly property int standardRadius: 4
    readonly property int sidebarWidth: 240  // 更宽的侧边栏

    RowLayout {
        id: browserLayout
        anchors.fill: parent
        spacing: 0
        // Element Plus 风格侧边栏
        Rectangle {
            id: leftMenuBar
            color: GTheme.dark ? GTheme.fillBase : GTheme.bgPage
            Layout.fillHeight: true
            Layout.minimumWidth: control.sidebarWidth
            Layout.preferredWidth: control.sidebarWidth
            Layout.maximumWidth: control.sidebarWidth

            // Element Plus 风格右侧分隔线
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: GTheme.borderBase
            }
            // Element Plus 风格标题
            Text {
                id: title
                text: qsTr("Preferences")
                anchors.left: parent.left
                anchors.leftMargin: control.standardSpacing * 1.5  // 24px
                anchors.top: parent.top
                anchors.topMargin: control.standardSpacing * 2     // 32px
                font.pixelSize: 18
                font.weight: Font.Medium
                color: GTheme.textPrimary
            }
            // Element Plus 风格导航菜单
            ColumnLayout {
                id: bar
                anchors.top: title.bottom
                anchors.topMargin: control.standardSpacing * 1.5  // 24px
                anchors.left: parent.left
                anchors.leftMargin: control.standardSpacing
                anchors.right: parent.right
                anchors.rightMargin: control.standardSpacing
                spacing: 4  // Element Plus 菜单项间距
                property int currentIndex: 0
                property var buttonsArr: [basic, advanced, lab]
                ButtonGroup{
                    id:titleGroup
                    onCheckedButtonChanged: {
                        let index  = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                        bar.currentIndex = index
                        console.debug("index ",index)
                    }
                }
                // Element Plus 风格导航项
                GButton {
                    variant: "nav"
                    id: basic
                    checkable: true
                    checked: true
                    ButtonGroup.group: titleGroup
                    iconSource: SegoeFluentIcons.PlayerSettings
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.navItemHeight
                    text: qsTr("Basic")
                    onClicked: { checked = true }
                }

                GButton {
                    variant: "nav"
                    id: advanced
                    checkable: true
                    ButtonGroup.group: titleGroup
                    iconSource: SegoeFluentIcons.KeyboardSettings
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.navItemHeight
                    text: qsTr("Advanced")
                    onClicked: { checked = true }
                }

                GButton {
                    variant: "nav"
                    id: lab
                    checkable: true
                    ButtonGroup.group: titleGroup
                    iconSource: SegoeFluentIcons.ExploitProtectionSettings
                    Layout.fillWidth: true
                    Layout.preferredHeight: GTheme.navItemHeight
                    text: qsTr("Lab")
                    onClicked: { checked = true }
                }
            }
        }

        // Element Plus 风格主内容区域
        Rectangle {
            id: browser
            color: GTheme.dark ? GTheme.bgPage : GTheme.bgWhite
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 480  // 更宽的最小宽度

            // Element Plus 风格容器背景
            Rectangle {
                anchors.fill: parent
                anchors.margins: control.standardSpacing
                color: GTheme.dark ? GTheme.fillBase : GTheme.bgWhite
                radius: control.standardRadius

                // 微妙的阴影效果
                layer.enabled: !GTheme.dark
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: Qt.rgba(0, 0, 0, 0.08)
                    shadowBlur: 8
                    shadowVerticalOffset: 2
                }
            }
            SettingPageTitle {
                id: settingTitle
                type: bar.currentIndex
                anchors.top: parent.top
                anchors.topMargin: control.standardSpacing * 2  // 32px
                anchors.left: parent.left
                anchors.leftMargin: control.standardSpacing * 2
                anchors.right: parent.right
                anchors.rightMargin: control.standardSpacing * 2
            }

            StackLayout {
                id: settingsStack
                anchors.top: settingTitle.bottom
                anchors.topMargin: control.standardSpacing
                anchors.left: parent.left
                anchors.leftMargin: control.standardSpacing * 2
                anchors.right: parent.right
                anchors.rightMargin: control.standardSpacing * 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: control.standardSpacing * 2
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
