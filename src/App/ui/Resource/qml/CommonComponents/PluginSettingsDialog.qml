import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// 插件配置对话框:声明式 Schema 渲染,市场卡片与网盘解析页共用
GDialogShell {
    id: dialog

    property string pluginName: ""
    readonly property real outerMargin: GTheme.spaceLG

    width: Math.min(520, parent ? Math.max(0, parent.width - outerMargin * 2) : 520)
    height: Math.min(560, parent ? Math.max(0, parent.height - outerMargin * 2) : 560)

    title: pluginName.length > 0 ? (PluginConfigManager.pluginInfo(pluginName).displayName || pluginName) : ""
    subtitle: qsTr("Plugin settings")
    iconName: "settings"

    function openFor(name) {
        pluginName = name
        settingsForm.pluginName = name
        settingsForm.reload()
        open()
    }

    Item {
        anchors.fill: parent

        ScrollView {
            id: bodyScroll
            anchors.fill: parent
            anchors.margins: GTheme.spaceLG
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: bodyScroll.availableWidth
                spacing: GTheme.spaceMD

                PluginSettingsForm {
                    id: settingsForm
                    Layout.fillWidth: true
                }

                AlertTip {
                    Layout.fillWidth: true
                    visible: settingsForm.hasMissingRequired
                    severity: "danger"
                    text: qsTr("Please fill in all required fields.")
                }
            }
        }
    }

    footer: Item {
        implicitHeight: footerRow.implicitHeight + GTheme.spaceMD * 2

        RowLayout {
            id: footerRow
            anchors.fill: parent
            anchors.leftMargin: GTheme.spaceLG
            anchors.rightMargin: GTheme.spaceLG
            anchors.topMargin: GTheme.spaceMD
            anchors.bottomMargin: GTheme.spaceMD
            spacing: GTheme.spaceSM

            GButton {
                type: 3
                buttonType: "danger"
                text: qsTr("Clear")
                Accessible.name: qsTr("Clear plugin settings")
                onClicked: {
                    PluginConfigManager.clear(dialog.pluginName)
                    settingsForm.reload()
                }
            }

            Item { Layout.fillWidth: true }

            GButton {
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }

            GButton {
                type: 1
                text: qsTr("Save")
                onClicked: {
                    var collected = settingsForm.collect()
                    if (collected === null)
                        return
                    if (!PluginConfigManager.save(dialog.pluginName, collected)) {
                        ToastManager.ShowError(qsTr("Failed to save plugin settings. Check disk permissions and retry."))
                        return
                    }
                    ToastManager.ShowSuccess(qsTr("Plugin settings saved."))
                    dialog.close()
                }
            }
        }
    }
}
