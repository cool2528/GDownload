import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import gdl.sdk

/**
 * CloseConfirmDialog - Close confirmation dialog
 *
 * Features:
 * - Ask user whether to quit or minimize to tray
 * - "Don't ask again" option
 * - Integrated with GTheme system
 */
Dialog {
    id: root

    title: qsTr("Close Confirmation")
    modal: true

    // Center in parent
    x: (mainWindow.width - width) / 2
    y: (mainWindow.height - height) / 2

    width: 420
    height: 420
    
    // Close action enum
    enum CloseAction {
        Cancel,         // Cancel
        Quit,          // Quit application
        MinimizeToTray // Minimize to system tray
    }

    // Signal: User made a selection
    signal actionSelected(int action, bool dontAskAgain)
    
    // Background style
    background: Rectangle {
        color: GTheme.bgWhite
        radius: 8
        border.width: 1
        border.color: GTheme.borderLight

        layer.enabled: true
        layer.effect: DropShadow {
            radius: 16
            samples: 33
            color: Qt.rgba(0, 0, 0, 0.15)
            horizontalOffset: 0
            verticalOffset: 4
        }
    }

    // Header style
    header: Rectangle {
        height: 50
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 12

            // Icon
            FontIcon {
                iconSource: SegoeFluentIcons.Info
                iconSize: 20
                color: GTheme.warningColor
                Layout.alignment: Qt.AlignVCenter
            }

            // Title text
            Label {
                text: root.title
                font.pixelSize: 16
                font.bold: true
                color: GTheme.textPrimary
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // Bottom divider
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: GTheme.borderLighter
        }
    }
    
    // Content area
    // 使用 Item 包裹 ColumnLayout 以添加内边距
    contentItem: Item {
        implicitWidth: columnLayout.implicitWidth + 40  // 左右各20px
        implicitHeight: columnLayout.implicitHeight + 20  // 上下各10px

        ColumnLayout {
            id: columnLayout
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 16

            // Prompt message
            Label {
                text: qsTr("Do you want to quit the application or minimize it to the system tray?")
                font.pixelSize: 14
                color: GTheme.textRegular
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // Description text
            Label {
                text: qsTr("Tip: When minimized to tray, the application will continue running in the background. You can restore the window from the system tray.")
                font.pixelSize: 12
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // "Don't ask again" option
            GCheckBox {
                id: dontAskAgainCheckbox
                text: qsTr("Don't ask again, remember my choice")
                checked: false
                Layout.topMargin: 5
            }

            // Button area
            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Layout.topMargin: 15

                Item {
                    Layout.fillWidth: true
                }

                // Cancel button
                GButton {
                    text: qsTr("Cancel")
                    implicitWidth: 90
                    implicitHeight: 32
                    onClicked: {
                        root.actionSelected(CloseConfirmDialog.Cancel, false)
                        root.close()
                    }
                }

                // Minimize to tray button
                GButton {
                    text: qsTr("Minimize to Tray")
                    implicitWidth: 120
                    implicitHeight: 32
                    onClicked: {
                        root.actionSelected(
                                    CloseConfirmDialog.MinimizeToTray,
                                    dontAskAgainCheckbox.checked
                                    )
                        root.close()
                    }
                }

                // Quit button
                GButton {
                    text: qsTr("Quit")
                    implicitWidth: 90
                    implicitHeight: 32
                    property color buttonColor: GTheme.dangerColor

                    // Custom style to make it stand out
                    background: Rectangle {
                        color: parent.hovered ? Qt.darker(parent.buttonColor, 1.1) : parent.buttonColor
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        root.actionSelected(
                                    CloseConfirmDialog.Quit,
                                    dontAskAgainCheckbox.checked
                                    )
                        root.close()
                    }
                }
            }
        }
    }

    // Reset "don't ask again" when opened
    onOpened: {
        dontAskAgainCheckbox.checked = false
    }
}
