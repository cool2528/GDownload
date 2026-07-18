import QtQuick
import QtQuick.Controls
import gdl.sdk

SpinBox {
    id: control

    value: 50
    editable: true
    LayoutMirroring.enabled: false

    property string size: "default"
    property string status: "normal"

    readonly property int implicitH: size === "large" ? 40 : (size === "small" ? 24 : 32)
    readonly property int radiusPx: GTheme.radiusBase
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

    implicitWidth: 136
    implicitHeight: Math.max(GTheme.sizeLarge, implicitH)
    leftPadding: GTheme.sizeLarge
    rightPadding: GTheme.sizeLarge
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: control.enabled ? GTheme.textRegular : GTheme.textDisabled
        selectionColor: GTheme.primaryColor
        selectedTextColor: GTheme.textInverse
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        selectByMouse: true
    }

    down.indicator: Rectangle {
        x: 1
        y: 1
        width: GTheme.sizeLarge
        height: control.height - 2
        radius: control.radiusPx
        color: {
            if (!control.enabled)
                return "transparent"
            if (control.down.pressed)
                return GTheme.fillBase
            if (control.down.hovered)
                return GTheme.fillLight
            return "transparent"
        }

        Rectangle {
            width: 10
            height: 2
            anchors.centerIn: parent
            radius: 1
            color: control.enabled ? GTheme.textSecondary : GTheme.textDisabled
        }

        Rectangle {
            width: 1
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            color: GTheme.borderLight
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
        }
    }

    up.indicator: Rectangle {
        x: control.width - width - 1
        y: 1
        width: GTheme.sizeLarge
        height: control.height - 2
        radius: control.radiusPx
        color: {
            if (!control.enabled)
                return "transparent"
            if (control.up.pressed)
                return GTheme.fillBase
            if (control.up.hovered)
                return GTheme.fillLight
            return "transparent"
        }

        Rectangle {
            width: 10
            height: 2
            anchors.centerIn: parent
            radius: 1
            color: control.enabled ? GTheme.textSecondary : GTheme.textDisabled
        }

        Rectangle {
            width: 2
            height: 10
            anchors.centerIn: parent
            radius: 1
            color: control.enabled ? GTheme.textSecondary : GTheme.textDisabled
        }

        Rectangle {
            width: 1
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            color: GTheme.borderLight
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
        }
    }

    background: Rectangle {
        implicitWidth: 136
        implicitHeight: Math.max(GTheme.sizeLarge, control.implicitH)
        radius: control.radiusPx
        color: !control.enabled || !control.editable ? GTheme.fillLighter : GTheme.surfaceBase
        border.width: control.hasError || control.hasSuccess || control.status === "warning" ? 2 : 1
        border.color: {
            if (!control.enabled)
                return GTheme.borderLight
            if (control.hasError || control.hasSuccess || control.status === "warning")
                return control.semanticBorderColor
            if (control.hovered)
                return GTheme.brandHover
            return !control.editable ? GTheme.borderLight : GTheme.borderBase
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
}
