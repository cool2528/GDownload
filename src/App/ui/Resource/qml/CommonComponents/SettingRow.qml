import QtQuick
import QtQuick.Layouts
import gdl.sdk

// Aurora 设置项行：说明与控件分栏；窄宽度时自动改为上下布局。
Item {
    id: root

    property string label: ""
    property string hint: ""
    property string statusText: ""
    property color statusColor: GTheme.textDanger
    property int labelWidth: 220
    property Item control: null

    readonly property bool wideLayout: width >= 520

    implicitWidth: 520
    implicitHeight: Math.max(64, layout.implicitHeight + GTheme.spaceMD * 2)
    Layout.minimumHeight: implicitHeight

    Rectangle {
        anchors.fill: parent
        radius: GTheme.radiusMedium
        color: root.enabled ? GTheme.surfaceBase : GTheme.fillLighter
        border.width: 1
        border.color: GTheme.borderLighter
    }

    GridLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: GTheme.spaceMD
        columns: root.wideLayout ? 2 : 1
        columnSpacing: GTheme.spaceMD
        rowSpacing: GTheme.spaceSM

        ColumnLayout {
            id: copyGroup
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: root.wideLayout ? root.labelWidth : 0
            Layout.alignment: Qt.AlignTop
            spacing: GTheme.spaceXS

            Text {
                id: labelText
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.label
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
                elide: Text.ElideRight
                maximumLineCount: 1

                MouseArea {
                    anchors.fill: parent
                    enabled: root.enabled && root.control !== null
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: root.control.forceActiveFocus()
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                visible: root.hint.length > 0
                text: root.hint
                font.pixelSize: GTheme.fontCaption
                color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                visible: root.statusText.length > 0
                text: root.statusText
                font.pixelSize: GTheme.fontCaption
                color: root.enabled ? root.statusColor : GTheme.textDisabled
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
                Accessible.name: text
            }
        }

        RowLayout {
            id: controlSlot
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: GTheme.sizeDefault
            Layout.alignment: root.wideLayout ? Qt.AlignTop : Qt.AlignLeft
            spacing: 0
        }
    }

    onControlChanged: {
        if (root.control)
            root.control.parent = controlSlot
    }
}
