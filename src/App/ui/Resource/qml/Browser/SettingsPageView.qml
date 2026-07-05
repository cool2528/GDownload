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

    objectName: "settingsShell"

    // 设置页壳布局令牌:只做别名,数值仍由 GTheme 统一管理
    readonly property int shellSpacing: GTheme.spaceLG
    readonly property int shellRadius: GTheme.radiusRound
    readonly property int shellSidebarWidth: GTheme.sidebarWidth

    RowLayout {
        id: browserLayout
        anchors.fill: parent
        spacing: 0
        // Element Plus 风格侧边栏
        Rectangle {
            id: leftMenuBar
            objectName: "settingsSidebar"
            color: GTheme.bgPage
            Layout.fillHeight: true
            Layout.minimumWidth: control.shellSidebarWidth
            Layout.preferredWidth: control.shellSidebarWidth
            Layout.maximumWidth: control.shellSidebarWidth

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
                objectName: "settingsTitle"
                text: qsTr("Preferences")
                anchors.left: parent.left
                anchors.leftMargin: control.shellSpacing + GTheme.spaceSM
                anchors.top: parent.top
                anchors.topMargin: GTheme.space3XL
                font.pixelSize: GTheme.fontTitle
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
            }
            // Element Plus 风格导航菜单
            ColumnLayout {
                id: bar
                objectName: "settingsNav"
                anchors.top: title.bottom
                anchors.topMargin: GTheme.space2XL
                anchors.left: parent.left
                anchors.leftMargin: control.shellSpacing
                anchors.right: parent.right
                anchors.rightMargin: control.shellSpacing
                spacing: GTheme.spaceXS
                property int currentIndex: 0
                property var buttonsArr: [basic, advanced, lab]
                ButtonGroup{
                    id:titleGroup
                    onCheckedButtonChanged: {
                        let index  = bar.buttonsArr.indexOf(titleGroup.checkedButton)
                        bar.currentIndex = index
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
                anchors.margins: control.shellSpacing
                color: GTheme.bgWhite
                radius: control.shellRadius

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
                anchors.topMargin: GTheme.space3XL
                anchors.left: parent.left
                anchors.leftMargin: GTheme.space2XL
                anchors.right: parent.right
                anchors.rightMargin: GTheme.space2XL
            }

            StackLayout {
                id: settingsStack
                objectName: "settingsStack"
                anchors.top: settingTitle.bottom
                anchors.topMargin: control.shellSpacing
                anchors.left: parent.left
                anchors.leftMargin: GTheme.space2XL
                anchors.right: parent.right
                anchors.rightMargin: GTheme.space2XL
                anchors.bottom: parent.bottom
                anchors.bottomMargin: GTheme.space2XL
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
