import QtQuick
import QtQuick.Layouts
import gdl.sdk 1.0

// Aurora 设置分组卡：单层内边距、清晰的 elevated/base 表面层级和自适应正文高度。
GCard {
    id: root

    property string title: ""
    property string description: ""
    default property alias content: contentSlot.children

    readonly property bool hasHeader: title.length > 0 || description.length > 0

    outlined: true
    padding: GTheme.spaceLG
    radius: GTheme.radiusLarge
    hoverEnabled: false
    interactive: false
    variant: "default"
    shadow: false
    disabled: !enabled

    implicitWidth: Math.max(560, contentItem ? contentItem.implicitWidth + root.padding * 2 : 0)
    implicitHeight: Math.max(120, contentItem ? contentItem.implicitHeight + root.padding * 2 : 0)

    background: Rectangle {
        radius: root.radius
        color: root.enabled ? GTheme.surfaceElevated : GTheme.fillLighter
        border.width: 1
        border.color: GTheme.borderLight

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
    }

    contentItem: ColumnLayout {
        id: cardBody
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: root.hasHeader ? GTheme.spaceLG : 0

        ColumnLayout {
            id: headerGroup
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS
            visible: root.hasHeader

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                visible: root.title.length > 0
                text: root.title
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightDemiBold
                color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                visible: root.description.length > 0
                text: root.description
                font.pixelSize: GTheme.fontCaption
                color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            id: contentSlot
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD
        }
    }
}
