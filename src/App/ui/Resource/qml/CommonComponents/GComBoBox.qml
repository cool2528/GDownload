import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

ComboBox {
    id: control

    // Public compatibility API.
    property string size: "default"
    property string status: "normal"
    property bool clearable: false
    property string placeholder: qsTr("Please select")
    property int maxPopHeight: 200
    // ComboBox has no native readOnly state. The overlay below preserves
    // legibility and focus while suppressing pointer activation.
    property bool readOnly: false

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
    rightPadding: GTheme.space3XL
    spacing: GTheme.spaceSM
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    delegate: ItemDelegate {
        id: delegateItem

        width: control.width
        height: GTheme.sizeLarge
        hoverEnabled: true
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: control.textRole
                ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole])
                : modelData
            leftPadding: GTheme.spaceMD
            rightPadding: GTheme.spaceMD
            font.pixelSize: control.fontPx
            color: control.currentIndex === index || control.highlightedIndex === index
                   ? GTheme.primaryColor : GTheme.textRegular
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: GTheme.radiusBase
            color: {
                if (delegateItem.highlighted || delegateItem.hovered)
                    return GTheme.fillLight
                if (control.currentIndex === index)
                    return GTheme.fillLighter
                return "transparent"
            }

            Behavior on color {
                ColorAnimation { duration: GTheme.durationFast; easing.type: GTheme.easingStandard }
            }
        }
    }

    indicator: Canvas {
        id: chevron

        x: control.width - width - GTheme.spaceMD
        y: (control.height - height) / 2
        width: 12
        height: 7
        contextType: "2d"

        onPaint: {
            context.reset()
            context.beginPath()
            context.moveTo(1, 1)
            context.lineTo(width / 2, height - 1)
            context.lineTo(width - 1, 1)
            context.lineWidth = 1.6
            context.lineCap = "round"
            context.lineJoin = "round"
            context.strokeStyle = control.enabled ? GTheme.textSecondary : GTheme.textDisabled
            context.stroke()
        }

        Connections {
            target: control
            function onEnabledChanged() { chevron.requestPaint() }
            function onPressedChanged() { chevron.requestPaint() }
        }

        Connections {
            target: GTheme
            function onDarkChanged() { chevron.requestPaint() }
        }
    }

    contentItem: Text {
        text: control.displayText || control.placeholder
        font.pixelSize: control.fontPx
        color: {
            if (!control.enabled)
                return GTheme.textDisabled
            if (!control.displayText)
                return GTheme.textPlaceholder
            return GTheme.textRegular
        }
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

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
            if (control.hovered && !control.readOnly)
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

    popup: Popup {
        y: control.height + GTheme.spaceXS
        width: control.width
        implicitHeight: Math.min(control.maxPopHeight, contentItem.implicitHeight + padding * 2)
        padding: GTheme.spaceXS

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < count)
                    positionViewAtIndex(currentIndex, ListView.Contain)
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: GTheme.bgOverlay
            border.color: GTheme.borderLight
            radius: GTheme.radiusMedium

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: GTheme.dark ? Qt.rgba(0, 0, 0, 0.42) : Qt.rgba(15 / 255, 23 / 255, 42 / 255, 0.16)
                shadowBlur: 16
                shadowVerticalOffset: 6
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        z: 10
        visible: control.readOnly
        acceptedButtons: Qt.AllButtons
        cursorShape: Qt.ArrowCursor
        onPressed: control.forceActiveFocus()
    }
}
