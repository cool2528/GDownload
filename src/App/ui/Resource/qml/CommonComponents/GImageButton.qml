import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

// Image button for title-bar and other image-pipeline actions.
Button {
    id: control

    property url normalImage
    property url hoverImage
    property color backgroundColor: "transparent"
    property color tintColor: "transparent"
    property size imageSize: Qt.size(width, height)

    readonly property bool hasHoverImage: hoverImage.toString().length > 0

    implicitWidth: GTheme.sizeLarge
    implicitHeight: GTheme.sizeLarge
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.56

    background: Rectangle {
        radius: GTheme.radiusBase
        color: control.backgroundColor

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
            ColorAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
        }
    }

    contentItem: Item {
        Image {
            id: image

            source: control.hovered && control.hasHoverImage ? control.hoverImage : control.normalImage
            anchors.centerIn: parent
            width: control.imageSize.width
            height: control.imageSize.height
            mipmap: true
            smooth: true
            fillMode: Image.PreserveAspectFit
            scale: control.down ? 0.9 : 1.0
            layer.enabled: !Qt.colorEqual(control.tintColor, "transparent")
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: control.tintColor
            }

            Behavior on scale {
                NumberAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
            }
        }
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
