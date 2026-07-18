import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk 1.0

// Aurora inline feedback. The historical severity/text/iconSource API remains
// available while title, description, semantic icons, and one recovery action
// provide the richer production alert anatomy.
Control {
    id: root

    property string severity: "info"
    property string text: ""
    property string title: ""
    property string description: ""
    property string actionText: ""
    property string actionLabel: ""
    property bool showIcon: true
    property bool showClose: false
    property int iconSource: 0
    property string iconName: ""

    signal actionTriggered()
    signal closeRequested()

    readonly property string resolvedSeverity: {
        const value = String(severity || "").toLowerCase()
        if (value === "success" || value === "warning" || value === "danger")
            return value
        return "info"
    }
    readonly property string resolvedDescription: description.length > 0 ? description : text
    readonly property string resolvedActionText: actionLabel.length > 0 ? actionLabel : actionText
    readonly property bool compactActionLayout: resolvedActionText.length > 0 && width < 520
    readonly property color alertBg: {
        switch (resolvedSeverity) {
        case "success": return GTheme.bgSuccess
        case "warning": return GTheme.bgWarning
        case "danger": return GTheme.bgDanger
        default: return GTheme.bgInfo
        }
    }
    readonly property color alertBorder: {
        switch (resolvedSeverity) {
        case "success": return GTheme.borderSuccess
        case "warning": return GTheme.borderWarning
        case "danger": return GTheme.borderDanger
        default: return GTheme.borderInfo
        }
    }
    readonly property color alertText: {
        switch (resolvedSeverity) {
        case "success": return GTheme.textSuccess
        case "warning": return GTheme.textWarning
        case "danger": return GTheme.textDanger
        default: return GTheme.textInfo
        }
    }
    readonly property color iconTint: {
        switch (resolvedSeverity) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "danger": return GTheme.dangerColor
        default: return GTheme.infoColor
        }
    }
    readonly property string defaultIconName: {
        switch (resolvedSeverity) {
        case "success": return "completed"
        case "warning": return "warning"
        case "danger": return "error"
        default: return "info"
        }
    }
    readonly property string resolvedIconName: iconName.length > 0 ? iconName : defaultIconName
    readonly property bool hasSemanticIcon: iconName.length > 0 || iconSource <= 0

    implicitWidth: 360
    implicitHeight: Math.max(GTheme.sizeLarge + padding * 2,
                             contentLayout.implicitHeight + padding * 2)
    padding: GTheme.spaceMD
    Accessible.role: Accessible.AlertMessage
    Accessible.name: title.length > 0 ? title : resolvedDescription
    Accessible.description: title.length > 0 ? resolvedDescription : ""

    background: Rectangle {
        radius: GTheme.radiusLarge
        color: root.alertBg
        border.width: 1
        border.color: root.alertBorder

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationBase }
        }
    }

    contentItem: RowLayout {
        id: contentLayout

        spacing: GTheme.spaceSM

        Item {
            visible: root.showIcon
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignTop

            Rectangle {
                anchors.fill: parent
                radius: GTheme.radiusMedium
                color: root.alertBg
                border.width: 1
                border.color: root.alertBorder
            }

            AuroraIcon {
                anchors.centerIn: parent
                visible: root.hasSemanticIcon
                name: root.resolvedIconName
                iconSize: GTheme.fontBody
                color: root.iconTint
            }

            FontIcon {
                anchors.centerIn: parent
                visible: !root.hasSemanticIcon && root.iconSource > 0
                iconSource: root.iconSource
                iconSize: GTheme.fontBody
                color: root.iconTint
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS

            Text {
                visible: root.title.length > 0
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.title
                color: root.alertText
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.resolvedDescription
                textFormat: Text.PlainText
                color: root.title.length > 0 ? GTheme.textRegular : root.alertText
                font.pixelSize: GTheme.fontCaption
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 5
                elide: Text.ElideRight

                Behavior on color {
                    ColorAnimation { duration: GTheme.durationBase }
                }
            }

            GButton {
                visible: root.compactActionLayout
                text: root.resolvedActionText
                buttonType: "default"
                size: "small"
                Layout.alignment: Qt.AlignLeft
                activeFocusOnTab: visible
                Accessible.name: text
                onClicked: root.actionTriggered()
            }
        }

        GButton {
            visible: root.resolvedActionText.length > 0 && !root.compactActionLayout
            text: root.resolvedActionText
            buttonType: "default"
            size: "small"
            Layout.alignment: Qt.AlignTop
            Layout.maximumWidth: 160
            activeFocusOnTab: visible
            Accessible.name: text
            onClicked: root.actionTriggered()
        }

        GButton {
            visible: root.showClose
            iconName: "close"
            iconSize: GTheme.fontBody
            imageSize: Qt.size(iconSize, iconSize)
            variant: "plain"
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignTop
            activeFocusOnTab: visible
            Accessible.name: qsTr("Dismiss alert")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Dismiss")
            onClicked: root.closeRequested()
        }
    }
}
