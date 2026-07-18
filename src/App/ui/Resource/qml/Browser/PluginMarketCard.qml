import QtQuick
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// 插件市场单卡：图标 + 名称/版本/作者 + 描述 + 域名标签 + 状态徽章 + 操作
// 对齐已评审的 Aurora 设计稿；颜色/间距/圆角取自 GTheme 令牌
Rectangle {
    id: card

    property string pluginName: ""
    property string displayName: ""
    property string description: ""
    property string author: ""
    property bool verified: false
    property string latestVersion: ""
    property string installedVersion: ""
    property int state: 0            // 0 Available 1 Installed 2 UpdateAvailable 3 Busy
    property bool enabledState: true
    property int progress: -1
    property string stage: ""
    property var tags: []
    property bool hasSettings: false

    signal install()
    signal updatePlugin()
    signal uninstall()
    signal toggleEnabled(bool on)
    signal openSettings()

    readonly property int stAvailable: 0
    readonly property int stInstalled: 1
    readonly property int stUpdate: 2
    readonly property int stBusy: 3

    radius: GTheme.radiusLarge
    color: GTheme.surfaceElevated
    border.width: 1
    border.color: GTheme.borderLight
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: GTheme.spaceLG
        spacing: GTheme.spaceMD

        // ---- 顶部：图标 + 名称/作者 + 状态徽章 ----
        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: GTheme.radiusMedium
                color: GTheme.primaryColor
                Text {
                    anchors.centerIn: parent
                    text: card.displayName.length > 0 ? card.displayName.charAt(0).toUpperCase() : "?"
                    color: GTheme.textInverse
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                RowLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceSM
                    Text {
                        text: card.displayName
                        font.pixelSize: GTheme.fontSubtitle
                        font.weight: GTheme.weightDemiBold
                        color: GTheme.textPrimary
                        elide: Text.ElideRight
                        Layout.maximumWidth: 180
                    }
                    Rectangle {
                        radius: GTheme.radiusSmall
                        color: GTheme.fillBase
                        implicitWidth: verText.implicitWidth + GTheme.spaceSM
                        implicitHeight: verText.implicitHeight + GTheme.spaceXS
                        Text {
                            id: verText
                            anchors.centerIn: parent
                            text: {
                                if (card.state === card.stUpdate)
                                    return card.installedVersion + " → " + card.latestVersion
                                if (card.state === card.stInstalled)
                                    return "v" + card.installedVersion
                                return "v" + card.latestVersion
                            }
                            font.pixelSize: GTheme.fontCaption
                            font.weight: GTheme.weightMedium
                            color: GTheme.textSecondary
                        }
                    }
                }

                RowLayout {
                    spacing: GTheme.spaceXS
                    AuroraIcon {
                        visible: card.verified
                        name: "lock"
                        iconSize: GTheme.fontCaption
                        color: GTheme.primaryColor
                    }
                    Text {
                        text: card.verified
                              ? qsTr("Verified") + " · " + card.author
                              : card.author + " · " + qsTr("Third-party")
                        font.pixelSize: GTheme.fontCaption
                        color: card.verified ? GTheme.textSecondary : GTheme.warningColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            // 状态徽章
            Rectangle {
                Layout.alignment: Qt.AlignTop
                radius: GTheme.radiusRound
                implicitWidth: stText.implicitWidth + GTheme.spaceMD
                implicitHeight: stText.implicitHeight + GTheme.spaceXS
                border.width: card.state === card.stAvailable ? 1 : 0
                border.color: GTheme.borderBase
                color: {
                    if (card.state === card.stInstalled) return GTheme.bgSuccess
                    if (card.state === card.stUpdate) return GTheme.bgWarning
                    return GTheme.fillBase
                }
                Text {
                    id: stText
                    anchors.centerIn: parent
                    text: {
                        if (card.state === card.stInstalled) return qsTr("Installed")
                        if (card.state === card.stUpdate) return qsTr("Update")
                        if (card.state === card.stBusy) return qsTr("Working")
                        return qsTr("Available")
                    }
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                    color: {
                        if (card.state === card.stInstalled) return GTheme.textSuccess
                        if (card.state === card.stUpdate) return GTheme.textWarning
                        return GTheme.textSecondary
                    }
                }
            }
        }

        // ---- 描述 ----
        Text {
            Layout.fillWidth: true
            text: card.description
            font.pixelSize: GTheme.fontBody
            color: GTheme.textRegular
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        // ---- 域名标签 ----
        Flow {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS
            Repeater {
                model: card.tags
                Rectangle {
                    radius: GTheme.radiusBase
                    color: GTheme.fillLight
                    border.width: 1
                    border.color: GTheme.borderLight
                    implicitWidth: tagText.implicitWidth + GTheme.spaceMD
                    implicitHeight: tagText.implicitHeight + GTheme.spaceXS
                    Text {
                        id: tagText
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // ---- 底部操作区 ----
        // 忙碌态：不确定进度条 + 阶段文案（插件体积小、操作瞬时，用动画态而非跳变百分比）
        RowLayout {
            visible: card.state === card.stBusy
            Layout.fillWidth: true
            spacing: GTheme.spaceSM
            GProgressBar {
                Layout.fillWidth: true
                indeterminate: true
            }
            Text {
                text: card.stage.length > 0 ? card.stage : qsTr("Working…")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
            }
        }

        // 非忙碌态：左侧启用开关，右侧操作按钮（主操作在最右，符合惯例）
        RowLayout {
            visible: card.state !== card.stBusy
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            // 已安装/可更新：启用开关在左
            RowLayout {
                visible: card.state === card.stInstalled || card.state === card.stUpdate
                spacing: GTheme.spaceXS
                GButtonSwitch {
                    checked: card.enabledState
                    onToggled: card.toggleEnabled(checked)
                }
                Text {
                    text: card.enabledState ? qsTr("Enabled") : qsTr("Disabled")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                }
            }

            Item { Layout.fillWidth: true }

            // 右侧操作：设置/移除（次要）在前，安装/更新（主操作）在最右
            GButton {
                visible: (card.state === card.stInstalled || card.state === card.stUpdate) && card.hasSettings
                type: 3
                iconName: "settings"
                text: qsTr("Settings")
                Accessible.name: qsTr("Plugin settings")
                onClicked: card.openSettings()
            }

            GButton {
                visible: card.state === card.stInstalled || card.state === card.stUpdate
                type: 3
                buttonType: "danger"
                iconName: "delete"
                text: qsTr("Remove")
                onClicked: card.uninstall()
            }

            GButton {
                visible: card.state === card.stAvailable
                type: 1
                iconName: "download"
                text: qsTr("Install")
                onClicked: card.install()
            }

            GButton {
                visible: card.state === card.stUpdate
                type: 1
                iconName: "refresh"
                text: qsTr("Update")
                onClicked: card.updatePlugin()
            }
        }
    }
}
