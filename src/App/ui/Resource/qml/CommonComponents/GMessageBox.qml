import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import gdl.sdk

/**
 * Aurora responsive message box.
 *
 * The public message/buttons/result contract remains compatible with the
 * original component. Preferred geometry is bounded by the popup parent, and
 * long copy scrolls instead of escaping the dialog surface.
 */
Dialog {
    id: root

    property string message: ""

    enum MessageType {
        Info,
        Warning,
        Error,
        Success,
        Question
    }

    property int messageType: GMessageBox.Info

    // Legacy glyph override plus the preferred Aurora semantic override.
    property int customIcon: -1
    property string customIconName: ""
    property Component customContent: null
    property var buttons: []
    property int defaultButtonIndex: -1

    // Preferred geometry. Actual geometry is bounded by the current parent.
    property int dialogWidth: 440
    property int standardHeight: 280

    readonly property int standardPadding: GTheme.spaceXL
    readonly property int standardSpacing: GTheme.spaceLG
    readonly property real outerMargin: GTheme.spaceLG
    readonly property real availableDialogWidth: parent
                                                  ? Math.max(0, parent.width - outerMargin * 2)
                                                  : dialogWidth
    readonly property real availableDialogHeight: parent
                                                   ? Math.max(0, parent.height - outerMargin * 2)
                                                   : standardHeight
    readonly property real preferredButtonsWidth: {
        var total = 0
        for (var i = 0; i < buttons.length; ++i) {
            total += buttons[i].width || 90
        }
        return total + Math.max(0, buttons.length - 1) * GTheme.spaceSM
    }
    readonly property bool stackButtons: width < 400
                                         || preferredButtonsWidth > width - standardPadding * 2
    readonly property real desiredHeight: messageHeader.implicitHeight
                                          + bodyColumn.implicitHeight
                                          + standardPadding * 2
                                          + (buttons.length > 0
                                             ? buttonGrid.implicitHeight + standardPadding * 2 + 1
                                             : 0)

    signal buttonClicked(int index, var buttonData)

    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape
    x: parent ? Math.max(0, Math.round((parent.width - width) / 2)) : 0
    y: parent ? Math.max(0, Math.round((parent.height - height) / 2)) : 0
    width: Math.min(dialogWidth,
                    availableDialogWidth > 0 ? availableDialogWidth : dialogWidth)
    height: Math.min(availableDialogHeight > 0 ? availableDialogHeight : standardHeight,
                     Math.max(Math.min(standardHeight,
                                       availableDialogHeight > 0
                                       ? availableDialogHeight : standardHeight),
                              desiredHeight))

    Overlay.modal: Rectangle {
        color: GTheme.overlayScrim
    }

    function getTypeIconName() {
        if (customIconName.length > 0) {
            return customIconName
        }

        switch(messageType) {
            case GMessageBox.Warning:
                return "warning"
            case GMessageBox.Error:
                return "error-badge"
            case GMessageBox.Success:
                return "completed"
            case GMessageBox.Question:
                return "help"
            default:
                return "info"
        }
    }

    function getTypeColor() {
        switch(messageType) {
            case GMessageBox.Warning:
                return GTheme.warningColor
            case GMessageBox.Error:
                return GTheme.dangerColor
            case GMessageBox.Success:
                return GTheme.successColor
            case GMessageBox.Question:
                return GTheme.primaryColor
            default:
                return GTheme.infoColor
        }
    }

    function getTypeBackground() {
        switch(messageType) {
            case GMessageBox.Warning:
                return GTheme.bgWarning
            case GMessageBox.Error:
                return GTheme.bgDanger
            case GMessageBox.Success:
                return GTheme.bgSuccess
            default:
                return GTheme.bgInfo
        }
    }

    function activateButton(index) {
        if (index < 0 || index >= buttons.length) {
            return
        }
        const buttonData = buttons[index]
        root.buttonClicked(index, buttonData)
        root.close()
    }

    function activateDefaultButton() {
        if (defaultButtonIndex >= 0 && defaultButtonIndex < buttons.length) {
            activateButton(defaultButtonIndex)
        }
    }

    function focusDefaultButton() {
        const focusIndex = defaultButtonIndex >= 0
                           && defaultButtonIndex < buttons.length
                           ? defaultButtonIndex : (buttons.length > 0 ? 0 : -1)
        const target = focusIndex >= 0 ? buttonRepeater.itemAt(focusIndex) : null
        if (target) {
            target.forceActiveFocus()
        } else {
            contentRoot.forceActiveFocus()
        }
    }

    background: Rectangle {
        color: GTheme.surfaceElevated
        radius: GTheme.radiusLarge
        border.width: 1
        border.color: GTheme.borderBase

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: GTheme.elevation4.color
            shadowBlur: Math.min(1.0, GTheme.elevation4.blur / 32)
            shadowHorizontalOffset: GTheme.elevation4.offsetX
            shadowVerticalOffset: GTheme.elevation4.offsetY
            autoPaddingEnabled: true
        }
    }

    header: Item {
        id: messageHeader
        implicitHeight: GTheme.titleBarHeight + GTheme.space2XL

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.standardPadding
            anchors.rightMargin: root.standardPadding
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.alignment: Qt.AlignVCenter
                radius: GTheme.radiusLarge
                color: root.getTypeBackground()

                AuroraIcon {
                    anchors.centerIn: parent
                    visible: root.customIconName.length > 0 || root.customIcon < 0
                    name: root.getTypeIconName()
                    iconSize: GTheme.fontTitle
                    color: root.getTypeColor()
                }

                FontIcon {
                    anchors.centerIn: parent
                    visible: root.customIconName.length === 0 && root.customIcon >= 0
                    iconSource: root.customIcon >= 0 ? root.customIcon : 0
                    iconSize: GTheme.fontTitle
                    color: root.getTypeColor()
                }
            }

            Label {
                text: root.title
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Divider {
            anchors.bottom: parent.bottom
            width: parent.width
        }
    }

    contentItem: ColumnLayout {
        id: contentRoot
        focus: true
        spacing: 0
        Accessible.role: Accessible.Dialog
        Accessible.name: root.title
        Accessible.description: root.message

        Keys.priority: Keys.AfterItem
        Keys.onReturnPressed: event => {
            root.activateDefaultButton()
            event.accepted = root.defaultButtonIndex >= 0
        }
        Keys.onEnterPressed: event => {
            root.activateDefaultButton()
            event.accepted = root.defaultButtonIndex >= 0
        }

        ScrollView {
            id: bodyScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: root.standardPadding
            clip: true
            contentWidth: availableWidth
            contentHeight: bodyColumn.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                id: bodyColumn
                width: bodyScroll.availableWidth
                spacing: root.standardSpacing

                Label {
                    id: messageLabel
                    objectName: "messageText"
                    text: root.message
                    font.pixelSize: GTheme.fontBody
                    lineHeight: 1.35
                    color: GTheme.textRegular
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    visible: root.message !== ""
                    Accessible.role: Accessible.StaticText
                    Accessible.name: text
                }

                Loader {
                    id: customContentLoader
                    sourceComponent: root.customContent
                    Layout.fillWidth: true
                    Layout.preferredHeight: item ? item.implicitHeight : 0
                    visible: root.customContent !== null
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            visible: root.buttons.length > 0
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: root.buttons.length > 0
                                    ? buttonGrid.implicitHeight + root.standardPadding * 2 : 0
            visible: root.buttons.length > 0

            GridLayout {
                id: buttonGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: root.standardPadding
                anchors.rightMargin: root.standardPadding
                columns: root.stackButtons ? 1 : Math.max(1, root.buttons.length)
                rowSpacing: GTheme.spaceSM
                columnSpacing: GTheme.spaceSM

                Repeater {
                    id: buttonRepeater
                    model: root.buttons

                    GButton {
                        required property int index
                        required property var modelData

                        objectName: modelData.objectName || ""
                        text: modelData.text || ""
                        Layout.fillWidth: root.stackButtons
                        Layout.preferredWidth: modelData.width || 90
                        Layout.preferredHeight: GTheme.sizeDefault
                        Accessible.name: text

                        buttonType: {
                            const type = modelData.type || "default"
                            if (type === "default" && index === root.defaultButtonIndex) {
                                return "primary"
                            }
                            return type
                        }

                        onClicked: root.activateButton(index)
                    }
                }
            }
        }
    }

    onOpened: Qt.callLater(focusDefaultButton)
}
