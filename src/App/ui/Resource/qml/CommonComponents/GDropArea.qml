import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import gdl.sdk
import "../Utils/utils.js" as Utils

// Aurora file drop target. The public path/accepted contract remains unchanged.
Control {
    id: control

    property string path: ""
    signal accepted()

    readonly property bool dragActive: dropZone.containsDrag
    readonly property bool pointerHovered: pointerArea.containsMouse
    readonly property bool interactionActive: enabled && (dragActive || pointerHovered)
    readonly property color resolvedFillColor: {
        if (!enabled)
            return GTheme.fillLighter
        if (dragActive)
            return GTheme.primaryLight(8)
        if (pointerHovered)
            return GTheme.primaryLight(9)
        return GTheme.surfaceElevated
    }
    readonly property color resolvedBorderColor: {
        if (!enabled)
            return GTheme.borderLight
        if (dragActive || pointerHovered || activeFocus)
            return GTheme.primaryColor
        return GTheme.borderBase
    }
    readonly property color resolvedContentColor: {
        if (!enabled)
            return GTheme.textDisabled
        if (dragActive || pointerHovered)
            return GTheme.primaryColor
        return GTheme.textSecondary
    }

    implicitWidth: 520
    implicitHeight: 150
    padding: 0
    hoverEnabled: enabled
    activeFocusOnTab: enabled
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.Button
    Accessible.name: dragActive
                     ? qsTr("Release to import torrent")
                     : qsTr("Choose a torrent or metalink file")
    Accessible.description: qsTr("Supports Torrent, Metalink, and Meta4 files")

    background: Rectangle {
        radius: GTheme.radiusLarge
        color: control.resolvedFillColor

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase; easing.type: GTheme.easingStandard }
        }
    }

    contentItem: Item {
        Canvas {
            id: outline

            anchors.fill: parent
            antialiasing: true

            onPaint: {
                const context = getContext("2d")
                const lineWidth = control.interactionActive ? 2 : 1
                context.clearRect(0, 0, width, height)
                context.setLineDash(control.dragActive ? [8, 4] : [5, 5])
                context.strokeStyle = control.resolvedBorderColor
                context.lineWidth = lineWidth
                context.beginPath()
                context.roundedRect(lineWidth / 2, lineWidth / 2,
                                     width - lineWidth, height - lineWidth,
                                     GTheme.radiusLarge, GTheme.radiusLarge)
                context.stroke()
            }

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            Connections {
                target: control
                function onResolvedBorderColorChanged() { outline.requestPaint() }
                function onInteractionActiveChanged() { outline.requestPaint() }
                function onDragActiveChanged() { outline.requestPaint() }
            }

            Connections {
                target: GTheme
                function onDarkChanged() { outline.requestPaint() }
            }

            Component.onCompleted: requestPaint()
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.max(0, Math.min(parent.width - GTheme.space2XL * 2, 440))
            spacing: GTheme.spaceXS

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                radius: GTheme.radiusMedium
                color: control.enabled
                       ? (control.interactionActive ? GTheme.primaryLight(8) : GTheme.fillLight)
                       : GTheme.fillBase

                Behavior on color {
                    ColorAnimation { duration: GTheme.durationBase }
                }

                AuroraIcon {
                    anchors.centerIn: parent
                    name: "cloud-download"
                    iconSize: GTheme.space2XL
                    color: control.resolvedContentColor
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.dragActive
                      ? qsTr("Release to import torrent")
                      : qsTr("Drag a torrent or metalink file here, or click to select.")
                color: control.resolvedContentColor
                font.pixelSize: GTheme.fontBody
                font.weight: control.dragActive ? GTheme.weightDemiBold : GTheme.weightMedium
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight

                Behavior on color {
                    ColorAnimation { duration: GTheme.durationBase }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Supports .torrent, .metalink, and .meta4 files")
                color: control.enabled ? GTheme.textSecondary : GTheme.textDisabled
                font.pixelSize: GTheme.fontCaption
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        DropArea {
            id: dropZone

            anchors.fill: parent
            enabled: control.enabled

            onEntered: drag => {
                if (drag.hasUrls)
                    drag.accept()
            }

            onDropped: drop => {
                if (!drop.hasUrls || drop.urls.length === 0)
                    return
                handleFile(drop.urls[0])
            }
        }

        MouseArea {
            id: pointerArea

            anchors.fill: parent
            enabled: control.enabled
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onPressed: control.forceActiveFocus()
            onClicked: fileDialog.open()
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: GTheme.radiusLarge + 2
        color: "transparent"
        border.width: 2
        border.color: GTheme.focusRing
        visible: control.enabled && control.activeFocus
        z: 2
    }

    Keys.onReturnPressed: event => {
        fileDialog.open()
        event.accepted = true
    }
    Keys.onEnterPressed: event => {
        fileDialog.open()
        event.accepted = true
    }
    Keys.onSpacePressed: event => {
        fileDialog.open()
        event.accepted = true
    }

    FileDialog {
        id: fileDialog

        title: qsTr("Please choose a torrent file")
        nameFilters: [
            qsTr("Torrent files (*.torrent)"),
            qsTr("Metalink Files (*.metalink)"),
            qsTr("Meta4 Files (*.meta4)")
        ]
        onAccepted: handleFile(currentFile)
    }

    function handleFile(fileUrl) {
        path = Utils.urlToLocalPath(fileUrl)
        control.accepted()
    }
}
