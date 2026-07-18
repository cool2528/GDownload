import QtQuick
import QtQuick.Layouts
import gdl.sdk

// Aurora 设置表单操作区：宽屏状态与按钮同排，窄屏自动堆叠，避免固定按钮宽度挤出卡片。
GridLayout {
    id: root

    property bool hasChanges: false
    property string statusText: ""
    property color statusColor: GTheme.textSecondary
    property string defaultStatusText: qsTr("Modify the values above and click 'Save Settings' to apply.")
    property int buttonWidth: 120
    readonly property bool stacked: width < 480

    signal reset()
    signal save()

    function applySettings(changes, successMessage) {
        var msg = (successMessage && successMessage.length > 0)
                  ? successMessage
                  : qsTr("Settings saved")
        var applied = []
        for (var i = 0; changes && i < changes.length; i++) {
            var c = changes[i]
            if (c.val !== c.old) {
                c.setter(c.val)
                applied.push(c.label + "=" + c.val)
            }
        }
        if (applied.length > 0) {
            ToastManager.ShowSuccess(msg, 3000)
            var summary = msg + ": " + applied.join(", ")
            root.statusText = summary
            root.statusColor = GTheme.successColor
            return summary
        }
        ToastManager.ShowInfo(qsTr("No changes detected."), 2000)
        root.statusText = qsTr("Settings unchanged.")
        root.statusColor = GTheme.textSecondary
        return root.statusText
    }

    Layout.fillWidth: true
    columns: stacked ? 1 : 3
    columnSpacing: GTheme.spaceMD
    rowSpacing: GTheme.spaceSM

    Text {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.alignment: Qt.AlignVCenter
        text: root.statusText.length > 0 ? root.statusText : root.defaultStatusText
        font.pixelSize: GTheme.fontCaption
        color: root.enabled ? root.statusColor : GTheme.textDisabled
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        maximumLineCount: root.stacked ? 3 : 2
        elide: Text.ElideRight
        Accessible.name: text

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
    }

    GButton {
        id: resetButton
        objectName: "btnCancel"
        text: qsTr("Reset")
        type: 3
        enabled: root.enabled
        Layout.fillWidth: root.stacked
        Layout.minimumWidth: root.stacked ? 0 : root.buttonWidth
        Layout.preferredWidth: root.stacked ? 0 : Math.max(root.buttonWidth, implicitWidth)
        onClicked: root.reset()
    }

    GButton {
        id: saveButton
        objectName: "btnSave"
        text: qsTr("Save Settings")
        type: 1
        enabled: root.enabled && root.hasChanges
        Layout.fillWidth: root.stacked
        Layout.minimumWidth: root.stacked ? 0 : root.buttonWidth
        Layout.preferredWidth: root.stacked ? 0 : Math.max(root.buttonWidth, implicitWidth)
        onClicked: root.save()
    }
}
