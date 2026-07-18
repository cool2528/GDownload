import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Aurora torrent file preview: preserves the FilePreviewModel contract while
// keeping the identity column elastic at both compact and dialog widths.
Rectangle {
    id: root

    property var previewModel
    signal clearRequested()

    readonly property bool compactLayout: width < 600
    readonly property int horizontalPadding: compactLayout ? GTheme.spaceSM : GTheme.spaceMD
    readonly property int columnSpacing: compactLayout ? GTheme.spaceXS : GTheme.spaceSM
    readonly property int iconColumnWidth: compactLayout ? GTheme.spaceLG : GTheme.space2XL
    readonly property int extensionColumnWidth: compactLayout ? 64 : 84
    readonly property int sizeColumnWidth: compactLayout ? 68 : 88

    implicitWidth: 520
    implicitHeight: 230
    color: GTheme.surfaceBase
    radius: GTheme.radiusMedium
    border.width: 1
    border.color: GTheme.borderLight
    clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault
            color: GTheme.surfaceElevated

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.horizontalPadding
                anchors.rightMargin: root.horizontalPadding
                spacing: root.columnSpacing

                GCheckBox {
                    id: selectAllCheckBox

                    Layout.preferredWidth: GTheme.sizeLarge
                    Layout.preferredHeight: GTheme.sizeLarge
                    checked: root.previewModel !== null
                             && fileList.count > 0
                             && root.previewModel.selectedCount === fileList.count
                    enabled: root.previewModel !== null && fileList.count > 0
                    Accessible.name: qsTr("Select all files")

                    onClicked: {
                        if (!root.previewModel)
                            return
                        if (checked)
                            root.previewModel.selectAll()
                        else
                            root.previewModel.unselectAll()
                        checked = Qt.binding(function() {
                            return root.previewModel !== null
                                   && fileList.count > 0
                                   && root.previewModel.selectedCount === fileList.count
                        })
                    }
                }

                Item {
                    Layout.preferredWidth: root.iconColumnWidth
                    Layout.preferredHeight: 1
                }

                Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("File Name")
                    color: GTheme.textRegular
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                    elide: Text.ElideRight
                }

                Label {
                    Layout.preferredWidth: root.extensionColumnWidth
                    Layout.minimumWidth: root.extensionColumnWidth
                    text: qsTr("Extension")
                    horizontalAlignment: Text.AlignLeft
                    color: GTheme.textRegular
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                    elide: Text.ElideRight
                }

                Label {
                    Layout.preferredWidth: root.sizeColumnWidth
                    Layout.minimumWidth: root.sizeColumnWidth
                    text: qsTr("Size")
                    horizontalAlignment: Text.AlignRight
                    color: GTheme.textRegular
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                }

                GButton {
                    id: clearButton

                    Layout.preferredWidth: GTheme.sizeDefault
                    Layout.preferredHeight: GTheme.sizeDefault
                    iconName: "delete"
                    iconSize: GTheme.fontBody
                    imageSize: Qt.size(iconSize, iconSize)
                    buttonType: "danger"
                    variant: "plain"
                    enabled: root.previewModel !== null
                    activeFocusOnTab: enabled
                    Accessible.name: qsTr("Clear torrent file")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Clear torrent file")
                    ToolTip.delay: 500

                    onClicked: root.clearRequested()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        ListView {
            id: fileList

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: GTheme.sizeLarge
            clip: true
            model: root.previewModel
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: fileRow

                required property int index
                required property var model

                width: fileList.width
                height: root.compactLayout ? GTheme.sizeDefault : GTheme.sizeLarge
                color: model.isSelected
                       ? (GTheme.dark ? GTheme.fillBase : GTheme.primaryLight(9))
                       : (rowHover.hovered ? GTheme.fillLighter : "transparent")

                Behavior on color {
                    ColorAnimation { duration: GTheme.durationFast }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: GTheme.borderLighter
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.horizontalPadding
                    anchors.rightMargin: root.horizontalPadding
                    spacing: root.columnSpacing

                    GCheckBox {
                        Layout.preferredWidth: GTheme.sizeLarge
                        Layout.preferredHeight: GTheme.sizeLarge
                        checked: fileRow.model.isSelected
                        Accessible.name: qsTr("Select %1").arg(fileRow.model.fileName)

                        onClicked: {
                            if (!root.previewModel)
                                return
                            root.previewModel.toggleSelection(fileRow.index)
                            checked = Qt.binding(function() {
                                return fileRow.model.isSelected
                            })
                        }
                    }

                    Item {
                        Layout.preferredWidth: root.iconColumnWidth
                        Layout.preferredHeight: GTheme.sizeLarge

                        Rectangle {
                            anchors.centerIn: parent
                            width: root.compactLayout ? GTheme.space2XL : GTheme.sizeDefault
                            height: width
                            radius: GTheme.radiusMedium
                            color: fileRow.model.isSelected
                                   ? (GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(8))
                                   : GTheme.fillLight

                            AuroraIcon {
                                anchors.centerIn: parent
                                name: "file"
                                iconSize: GTheme.fontBody
                                color: fileRow.model.isSelected ? GTheme.primaryColor : GTheme.textSecondary
                            }
                        }
                    }

                    Label {
                        id: fileNameLabel

                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: fileRow.model.fileName
                        color: GTheme.textPrimary
                        font.pixelSize: GTheme.fontCaption
                        font.weight: fileRow.model.isSelected ? GTheme.weightMedium : GTheme.weightRegular
                        elide: Text.ElideMiddle
                        maximumLineCount: 1
                        verticalAlignment: Text.AlignVCenter
                        ToolTip.visible: fileNameHover.hovered && truncated
                        ToolTip.text: text
                        ToolTip.delay: 500

                        HoverHandler {
                            id: fileNameHover
                            acceptedDevices: PointerDevice.Mouse
                        }
                    }

                    Label {
                        Layout.preferredWidth: root.extensionColumnWidth
                        Layout.minimumWidth: root.extensionColumnWidth
                        text: fileRow.model.fileExtension
                        horizontalAlignment: Text.AlignLeft
                        color: GTheme.textSecondary
                        font.pixelSize: GTheme.fontCaption
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Label {
                        Layout.preferredWidth: root.sizeColumnWidth
                        Layout.minimumWidth: root.sizeColumnWidth
                        text: fileRow.model.fileSize
                        horizontalAlignment: Text.AlignRight
                        color: GTheme.textRegular
                        font.pixelSize: GTheme.fontCaption
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Item {
                        Layout.preferredWidth: GTheme.sizeDefault
                        Layout.preferredHeight: 1
                    }
                }

                HoverHandler {
                    id: rowHover
                    acceptedDevices: PointerDevice.Mouse
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: GTheme.borderLight
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: GTheme.sizeDefault
            color: GTheme.surfaceElevated

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.horizontalPadding
                anchors.rightMargin: root.horizontalPadding
                spacing: GTheme.spaceSM

                AuroraIcon {
                    name: "completed"
                    iconSize: GTheme.fontBody
                    color: root.previewModel ? GTheme.primaryColor : GTheme.textDisabled
                }

                Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.previewModel
                          ? qsTr("Selected: %1 files, Total %2")
                              .arg(root.previewModel.selectedCount)
                              .arg(root.previewModel.totalSize)
                          : ""
                    color: GTheme.textSecondary
                    font.pixelSize: GTheme.fontCaption
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }
        }
    }
}
