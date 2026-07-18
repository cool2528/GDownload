import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Shared Aurora empty state for lists, tables, and product workspaces.
Control {
    id: control

    property string title: ""
    property string description: ""
    property string iconName: "info"
    property color accentColor: GTheme.primaryColor
    property string actionText: ""
    property bool compact: false
    property int maximumContentWidth: 420

    signal actionTriggered()

    readonly property bool hasAction: actionText.length > 0

    implicitWidth: Math.min(maximumContentWidth, contentLayout.implicitWidth)
    implicitHeight: contentLayout.implicitHeight
    padding: 0

    Accessible.name: title
    Accessible.description: description

    background: null

    contentItem: ColumnLayout {
        id: contentLayout

        spacing: control.compact ? GTheme.spaceSM : GTheme.spaceMD

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: control.compact ? GTheme.sizeLarge : 72
            Layout.preferredHeight: Layout.preferredWidth
            radius: GTheme.radiusRound
            color: GTheme.fillLighter
            border.width: 1
            border.color: GTheme.borderLighter

            AuroraIcon {
                anchors.centerIn: parent
                name: control.iconName
                iconSize: control.compact ? GTheme.fontSubtitle : GTheme.fontH1
                color: control.accentColor
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: control.title
            visible: text.length > 0
            color: GTheme.textPrimary
            font.pixelSize: control.compact ? GTheme.fontBody : GTheme.fontSubtitle
            font.weight: GTheme.weightDemiBold
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: control.description
            visible: text.length > 0
            color: GTheme.textSecondary
            font.pixelSize: GTheme.fontBody
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }

        GButton {
            Layout.alignment: Qt.AlignHCenter
            visible: control.hasAction
            text: control.actionText
            buttonType: "primary"
            Accessible.name: control.actionText
            onClicked: control.actionTriggered()
        }
    }
}
