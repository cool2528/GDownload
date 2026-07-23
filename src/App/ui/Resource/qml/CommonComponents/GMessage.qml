import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import gdl.sdk

// Aurora transient message. It remains dynamically created by ToastContainer
// and preserves the historical show/close/timer/type API without using Popup
// coordinates for stack layout.
Control {
    id: root

    property string message: ""
    property string title: ""
    property int duration: 3000
    property int messageId: -1
    property int maxWidth: 420
    property int minWidth: 320
    property int maxTextLines: 3
    property bool showClose: false
    property bool plain: false
    property int customIcon: -1
    property string iconName: ""
    property int offset: GTheme.spaceLG
    property bool closing: false

    readonly property int standardPadding: GTheme.spaceMD
    readonly property int iconSize: GTheme.fontBody
    readonly property int closeButtonSize: GTheme.sizeLarge

    enum MessageType {
        Primary,
        Success,
        Warning,
        Info,
        Error
    }
    property int messageType: GMessage.Info

    enum Placement {
        Top,
        TopLeft,
        TopRight,
        Bottom,
        BottomLeft,
        BottomRight
    }
    property int placement: GMessage.Top

    signal messageClosed(int messageId)

    visible: false
    opacity: 0
    padding: standardPadding
    width: parent ? Math.min(maxWidth, parent.width) : implicitWidth
    implicitWidth: Math.max(minWidth, Math.min(maxWidth, 360))
    implicitHeight: Math.max(GTheme.sizeLarge + padding * 2,
                             messageLayout.implicitHeight + padding * 2)
    height: implicitHeight
    Accessible.role: Accessible.AlertMessage
    Accessible.name: title.length > 0 ? title : message
    Accessible.description: title.length > 0 ? message : ""

    function isTopPlacement() {
        return placement === GMessage.Top
               || placement === GMessage.TopLeft
               || placement === GMessage.TopRight
    }

    function getBackgroundColor() {
        if (plain)
            return GTheme.surfaceElevated
        switch (messageType) {
        case GMessage.Primary: return GTheme.primaryLight(9)
        case GMessage.Success: return GTheme.bgSuccess
        case GMessage.Warning: return GTheme.bgWarning
        case GMessage.Info: return GTheme.bgInfo
        case GMessage.Error: return GTheme.bgDanger
        default: return GTheme.surfaceElevated
        }
    }

    function getBorderColor() {
        switch (messageType) {
        case GMessage.Primary: return GTheme.primaryColor
        case GMessage.Success: return GTheme.borderSuccess
        case GMessage.Warning: return GTheme.borderWarning
        case GMessage.Info: return GTheme.borderInfo
        case GMessage.Error: return GTheme.borderDanger
        default: return GTheme.borderLight
        }
    }

    function getIconName() {
        if (iconName.length > 0)
            return iconName
        switch (messageType) {
        case GMessage.Primary: return "completed"
        case GMessage.Success: return "completed"
        case GMessage.Warning: return "warning"
        case GMessage.Info: return "info"
        case GMessage.Error: return "error"
        default: return "info"
        }
    }

    function getIconColor() {
        switch (messageType) {
        case GMessage.Primary: return GTheme.primaryColor
        case GMessage.Success: return GTheme.successColor
        case GMessage.Warning: return GTheme.warningColor
        case GMessage.Info: return GTheme.infoColor
        case GMessage.Error: return GTheme.dangerColor
        default: return GTheme.textSecondary
        }
    }

    function getTextColor() {
        return plain ? getIconColor() : GTheme.textPrimary
    }

    function show(msg, type, time) {
        message = msg
        messageType = type
        closing = false
        if (time !== undefined)
            duration = time
        if (duration > 0 && String(msg).length > 50)
            duration = Math.min(duration * 1.5, 8000)

        exitAnimation.stop()
        visible = true
        opacity = 0
        slideOffset.y = isTopPlacement() ? -GTheme.spaceMD : GTheme.spaceMD
        enterAnimation.restart()
        if (duration > 0)
            closeTimer.restart()
    }

    function closeMessage() {
        if (closing)
            return
        closing = true
        closeTimer.stop()
        messageClosed(messageId)
        exitAnimation.restart()
    }

    function open() {
        show(message, messageType, duration)
    }

    function close() {
        closeMessage()
    }

    transform: Translate {
        id: slideOffset
    }

    background: Rectangle {
        color: root.plain ? "transparent" : GTheme.surfaceElevated
        radius: GTheme.radiusLarge
        border.width: 1
        border.color: root.getBorderColor()

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.getBackgroundColor()
            opacity: root.plain ? 0 : 0.42
        }

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationBase }
        }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: GTheme.elevation2.color
            shadowBlur: Math.min(1.0, GTheme.elevation2.blur / 32)
            shadowHorizontalOffset: GTheme.elevation2.offsetX
            shadowVerticalOffset: GTheme.elevation2.offsetY
            autoPaddingEnabled: true
        }
    }

    contentItem: RowLayout {
        id: messageLayout

        spacing: GTheme.spaceSM

        Item {
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignTop

            Rectangle {
                anchors.fill: parent
                radius: GTheme.radiusMedium
                color: root.getBackgroundColor()
                border.width: 1
                border.color: root.getBorderColor()
            }

            AuroraIcon {
                anchors.centerIn: parent
                visible: root.customIcon < 0 || root.iconName.length > 0
                name: root.getIconName()
                iconSize: root.iconSize
                color: root.getIconColor()
            }

            FontIcon {
                anchors.centerIn: parent
                visible: root.iconName.length === 0 && root.customIcon >= 0
                iconSource: root.customIcon >= 0 ? root.customIcon : 0
                iconSize: root.iconSize
                color: root.getIconColor()
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
                color: root.getTextColor()
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Label {
                id: messageLabel

                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.message
                // 提示内容可能来自网盘服务端返回的消息，禁用富文本解析以防内容伪装
                textFormat: Text.PlainText
                color: root.getTextColor()
                font.pixelSize: GTheme.fontCaption
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: root.maxTextLines
                elide: Text.ElideRight
                ToolTip.visible: messageHover.hovered && truncated
                ToolTip.text: root.message
                ToolTip.delay: 500

                HoverHandler {
                    id: messageHover
                    acceptedDevices: PointerDevice.Mouse
                }
            }
        }

        GButton {
            visible: root.showClose
            iconName: "close"
            iconSize: GTheme.fontBody
            imageSize: Qt.size(iconSize, iconSize)
            variant: "plain"
            Layout.preferredWidth: root.closeButtonSize
            Layout.preferredHeight: root.closeButtonSize
            Layout.alignment: Qt.AlignTop
            activeFocusOnTab: visible
            Accessible.name: qsTr("Dismiss notification")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Dismiss")
            onClicked: root.closeMessage()
        }
    }

    Timer {
        id: closeTimer
        interval: root.duration
        repeat: false
        onTriggered: root.closeMessage()
    }

    ParallelAnimation {
        id: enterAnimation

        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            duration: GTheme.durationSlow
            easing.type: GTheme.easingStandard
        }
        NumberAnimation {
            target: slideOffset
            property: "y"
            to: 0
            duration: GTheme.durationSlow
            easing.type: GTheme.easingStandard
        }
    }

    SequentialAnimation {
        id: exitAnimation

        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "opacity"
                to: 0
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
            NumberAnimation {
                target: slideOffset
                property: "y"
                to: root.isTopPlacement() ? -GTheme.spaceSM : GTheme.spaceSM
                duration: GTheme.durationBase
                easing.type: GTheme.easingStandard
            }
        }
        ScriptAction {
            script: {
                root.visible = false
                root.destroy()
            }
        }
    }
}
