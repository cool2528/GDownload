import QtQuick
import QtQuick.Controls
import gdl.sdk

TextField {
    id: control

    // Public compatibility API: large | default | small.
    property string size: "default"
    // normal | success | warning | danger | error.
    property string status: "normal"

    readonly property int implicitH: size === "large" ? 40 : (size === "small" ? 24 : 32)
    readonly property int radiusPx: GTheme.radiusBase
    readonly property int fontPx: size === "large" ? GTheme.fontSubtitle
                                                    : (size === "small" ? GTheme.fontCaption : GTheme.fontBody)
    readonly property bool hasError: status === "danger" || status === "error"
    readonly property bool hasSuccess: status === "success"
    readonly property color semanticBorderColor: {
        if (hasError)
            return GTheme.borderDanger
        if (hasSuccess)
            return GTheme.borderSuccess
        if (status === "warning")
            return GTheme.borderWarning
        return GTheme.borderBase
    }

    implicitWidth: 220
    implicitHeight: Math.max(GTheme.sizeLarge, implicitH)
    leftPadding: GTheme.spaceMD
    rightPadding: GTheme.spaceMD
    font.pixelSize: fontPx
    color: enabled ? GTheme.textRegular : GTheme.textDisabled
    placeholderTextColor: enabled ? GTheme.textPlaceholder : GTheme.textDisabled
    selectedTextColor: GTheme.textInverse
    selectionColor: GTheme.primaryColor
    selectByMouse: true
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        id: fieldSurface

        implicitWidth: 220
        implicitHeight: Math.max(GTheme.sizeLarge, control.implicitH)
        radius: control.radiusPx
        color: !control.enabled || control.readOnly ? GTheme.fillLighter : GTheme.surfaceBase
        border.width: control.hasError || control.hasSuccess || control.status === "warning" ? 2 : 1
        border.color: {
            if (!control.enabled)
                return GTheme.borderLight
            if (control.hasError || control.hasSuccess || control.status === "warning")
                return control.semanticBorderColor
            if (control.hovered)
                return GTheme.brandHover
            return control.readOnly ? GTheme.borderLight : GTheme.borderBase
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 2
            border.color: GTheme.focusRing
            visible: control.enabled && control.activeFocus
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
        }

        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
        }
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.IBeamCursor : Qt.ArrowCursor
    }
}
