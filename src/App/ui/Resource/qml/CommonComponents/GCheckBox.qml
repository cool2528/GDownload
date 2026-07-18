import QtQuick
import QtQuick.Controls
import gdl.sdk

CheckBox {
    id: control

    checked: false
    property string size: "default"
    property bool border: false
    property string status: "normal"

    readonly property int indicatorPx: size === "large" ? 18 : (size === "small" ? 14 : 16)
    readonly property int fontPx: size === "large" ? GTheme.fontSubtitle
                                                    : (size === "small" ? GTheme.fontCaption : GTheme.fontBody)
    readonly property int borderRadius: GTheme.radiusBase
    readonly property color accentColor: {
        if (status === "success")
            return GTheme.successColor
        if (status === "warning")
            return GTheme.warningColor
        if (status === "danger" || status === "error")
            return GTheme.dangerColor
        return GTheme.primaryColor
    }

    implicitHeight: GTheme.sizeLarge
    spacing: GTheme.spaceSM
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.56

    indicator: Item {
        implicitWidth: GTheme.sizeLarge
        implicitHeight: GTheme.sizeLarge
        x: control.leftPadding
        y: (control.height - height) / 2

        Rectangle {
            id: selectionBox

            width: control.indicatorPx
            height: control.indicatorPx
            anchors.centerIn: parent
            radius: control.borderRadius
            color: control.checked ? control.accentColor
                                   : (!control.enabled ? GTheme.fillLighter : GTheme.surfaceBase)
            border.width: 1
            border.color: {
                if (control.checked)
                    return control.accentColor
                if (control.hovered && control.enabled)
                    return control.accentColor
                return control.enabled ? GTheme.borderBase : GTheme.borderLight
            }
            scale: control.down ? 0.9 : 1.0

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.width: 2
                border.color: GTheme.focusRing
                visible: control.enabled && control.activeFocus
            }

            Canvas {
                id: checkmark

                anchors.fill: parent
                anchors.margins: 3
                visible: control.checked

                onPaint: {
                    const context = getContext("2d")
                    context.reset()
                    context.beginPath()
                    context.moveTo(width * 0.12, height * 0.52)
                    context.lineTo(width * 0.40, height * 0.78)
                    context.lineTo(width * 0.88, height * 0.22)
                    context.lineWidth = 2
                    context.lineCap = "round"
                    context.lineJoin = "round"
                    context.strokeStyle = GTheme.textInverse
                    context.stroke()
                }

                onVisibleChanged: requestPaint()

                Connections {
                    target: GTheme
                    function onDarkChanged() { checkmark.requestPaint() }
                }
            }

            Behavior on color {
                ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
            }

            Behavior on border.color {
                ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
            }

            Behavior on scale {
                NumberAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
            }
        }
    }

    contentItem: Text {
        text: control.text
        leftPadding: control.indicator.width + control.spacing
        font.pixelSize: control.fontPx
        font.weight: GTheme.weightRegular
        color: control.enabled ? GTheme.textRegular : GTheme.textDisabled
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        visible: control.border
        color: control.checked ? GTheme.fillLighter : "transparent"
        border.width: 1
        border.color: control.checked || control.hovered ? control.accentColor : GTheme.borderBase
        radius: GTheme.radiusMedium
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
