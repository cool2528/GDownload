import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Aurora theme option: a bounded, self-rendered product preview that does not
// depend on legacy bitmap assets. imageSource remains for source compatibility.
Button {
    id: control

    property url imageSource: ""
    property string tipText: ""
    property string description: ""
    property string previewMode: "system"

    readonly property bool showsLight: previewMode !== "dark"
    readonly property bool showsDark: previewMode !== "light"
    readonly property bool splitPreview: previewMode === "system"

    implicitWidth: 144
    implicitHeight: 144
    padding: 0
    spacing: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    Accessible.name: tipText
    Accessible.description: description

    background: Rectangle {
        radius: GTheme.radiusLarge
        color: {
            if (control.checked)
                return GTheme.dark ? GTheme.fillBase : GTheme.primaryLight(9)
            if (control.down)
                return GTheme.fillBase
            if (control.hovered)
                return GTheme.fillLighter
            return GTheme.surfaceBase
        }
        border.width: control.checked || control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? GTheme.focusRing
                                               : (control.checked ? GTheme.primaryColor : GTheme.borderLight)

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationFast }
        }
    }

    contentItem: Item {
        implicitWidth: 144
        implicitHeight: 144

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: GTheme.spaceSM
            spacing: GTheme.spaceSM

            Rectangle {
                id: previewFrame
                objectName: control.previewMode + "ThemePreview"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: 76
                radius: GTheme.radiusMedium
                color: "transparent"
                border.width: 1
                border.color: control.checked ? GTheme.primaryColor : GTheme.borderLighter
                clip: true

                Rectangle {
                    id: lightPane
                    visible: control.showsLight
                    x: 0
                    y: 0
                    width: control.splitPreview ? previewFrame.width / 2 : previewFrame.width
                    height: previewFrame.height
                    color: "#F3F6FB"
                    clip: true

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: 14
                        color: "#FFFFFF"
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.topMargin: 14
                        width: Math.max(13, parent.width * 0.24)
                        color: "#E8EEF7"

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 9
                            width: Math.max(5, parent.width - 8)
                            height: 4
                            radius: 2
                            color: "#409EFF"
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: Math.max(13, parent.width * 0.24) + 6
                        anchors.rightMargin: 6
                        anchors.topMargin: 21
                        anchors.bottomMargin: 7
                        radius: 5
                        color: "#FFFFFF"

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            height: 4
                            radius: 2
                            color: "#C6D2E3"
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: 6
                            anchors.rightMargin: Math.max(8, parent.width * 0.3)
                            anchors.topMargin: 16
                            height: 4
                            radius: 2
                            color: "#E3EAF4"
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.margins: 6
                            width: Math.max(12, parent.width * 0.36)
                            height: 7
                            radius: 3
                            color: "#409EFF"
                        }
                    }
                }

                Rectangle {
                    id: darkPane
                    visible: control.showsDark
                    x: control.splitPreview ? previewFrame.width / 2 : 0
                    y: 0
                    width: control.splitPreview ? previewFrame.width / 2 : previewFrame.width
                    height: previewFrame.height
                    color: "#080D18"
                    clip: true

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: 14
                        color: "#151F31"
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.topMargin: 14
                        width: Math.max(13, parent.width * 0.24)
                        color: "#101827"

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 9
                            width: Math.max(5, parent.width - 8)
                            height: 4
                            radius: 2
                            color: "#5AAEFF"
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: Math.max(13, parent.width * 0.24) + 6
                        anchors.rightMargin: 6
                        anchors.topMargin: 21
                        anchors.bottomMargin: 7
                        radius: 5
                        color: "#151F31"

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 6
                            height: 4
                            radius: 2
                            color: "#61708A"
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: 6
                            anchors.rightMargin: Math.max(8, parent.width * 0.3)
                            anchors.topMargin: 16
                            height: 4
                            radius: 2
                            color: "#34435E"
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.margins: 6
                            width: Math.max(12, parent.width * 0.36)
                            height: 7
                            radius: 3
                            color: "#5AAEFF"
                        }
                    }
                }

                Rectangle {
                    visible: control.splitPreview
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 1
                    height: parent.height
                    color: "#60718C"
                    opacity: 0.55
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: control.tipText.length > 0 ? control.tipText : control.text
                    color: control.checked ? GTheme.primaryColor : GTheme.textPrimary
                    font.pixelSize: GTheme.fontBody
                    font.weight: control.checked ? GTheme.weightDemiBold : GTheme.weightMedium
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Rectangle {
                    width: 16
                    height: 16
                    radius: GTheme.radiusCircle
                    color: control.checked ? GTheme.primaryColor : "transparent"
                    border.width: control.checked ? 0 : 1
                    border.color: GTheme.borderBase

                    Rectangle {
                        visible: control.checked
                        anchors.centerIn: parent
                        width: 6
                        height: 6
                        radius: GTheme.radiusCircle
                        color: GTheme.textInverse
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.description
                color: GTheme.textSecondary
                font.pixelSize: GTheme.fontCaption
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    HoverHandler {
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
