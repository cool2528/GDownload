import QtQuick
import QtQuick.Controls
import gdl.sdk

Switch {
    id: control

    property string size: "default"
    readonly property int trackH: size === "large" ? 32 : (size === "small" ? 24 : 28)
    readonly property int trackW: size === "large" ? 52 : (size === "small" ? 40 : 44)
    readonly property int knob: trackH - 8

    property color checkedBkColor: GTheme.primaryColor
    property color normalBkColor: GTheme.fillBase
    property color checkedFkColor: GTheme.surfaceBase
    property color normalFkColor: GTheme.surfaceBase
    property color textColor: GTheme.textPrimary

    readonly property bool usesDefaultCheckedColor: Qt.colorEqual(checkedBkColor, GTheme.primaryColor)
    readonly property color effectiveTrackColor: {
        if (!control.enabled)
            return GTheme.fillLighter
        if (control.checked) {
            if (control.down)
                return usesDefaultCheckedColor ? GTheme.brandPressed : Qt.darker(checkedBkColor, 1.14)
            if (control.hovered)
                return usesDefaultCheckedColor ? GTheme.brandHover : Qt.darker(checkedBkColor, 1.07)
            return checkedBkColor
        }
        if (control.down)
            return GTheme.fillBase
        if (control.hovered)
            return GTheme.fillLight
        return normalBkColor
    }

    implicitHeight: GTheme.sizeLarge
    spacing: GTheme.spaceSM
    font.pixelSize: GTheme.fontBody
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.56

    indicator: Item {
        implicitWidth: control.trackW
        implicitHeight: GTheme.sizeLarge
        x: control.width - width - control.rightPadding
        y: (control.height - height) / 2

        Rectangle {
            id: track

            width: control.trackW
            height: control.trackH
            anchors.centerIn: parent
            radius: height / 2
            color: control.effectiveTrackColor
            border.width: 1
            border.color: control.checked ? control.effectiveTrackColor : GTheme.borderBase

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.width: 2
                border.color: GTheme.focusRing
                visible: control.enabled && control.activeFocus
            }

            Rectangle {
                id: toggleButton

                x: control.checked ? parent.width - width - 4 : 4
                anchors.verticalCenter: parent.verticalCenter
                width: control.knob
                height: control.knob
                radius: height / 2
                color: control.checked ? control.checkedFkColor : control.normalFkColor
                scale: control.down ? 0.9 : 1.0

                Behavior on x {
                    NumberAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
                }

                Behavior on scale {
                    NumberAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
                }
            }

            Behavior on color {
                ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
            }

            Behavior on border.color {
                ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? control.textColor : GTheme.textDisabled
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        rightPadding: control.indicator.width + control.spacing
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
