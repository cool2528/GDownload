import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import gdl.sdk
import "../Utils/utils.js" as Utils

// Aurora 文件夹选择器：保留历史、路径编辑和原生目录选择行为，窄宽度下自动使用纯图标 Browse。
Item {
    id: folderSelector

    property string path: ""
    property bool readOnly: true
    property FolderHistoryModel historyModel: FolderHistoryModel {
        maxHistoryCount: 10
    }

    signal actived
    signal textChanged(string text)
    signal historySelected(string path)

    readonly property bool showBrowseLabel: width >= 360

    implicitWidth: 360
    implicitHeight: GTheme.sizeLarge
    Layout.minimumHeight: implicitHeight

    RowLayout {
        anchors.fill: parent
        spacing: GTheme.spaceSM

        GButton {
            id: historyBtn
            Layout.preferredWidth: GTheme.sizeLarge
            Layout.preferredHeight: GTheme.sizeLarge
            Layout.minimumWidth: GTheme.sizeLarge
            Layout.leftMargin: GTheme.spaceXS
            Layout.rightMargin: GTheme.spaceXS
            size: "large"
            variant: "plain"
            iconName: "history"
            imageSize: Qt.size(GTheme.fontSubtitle, GTheme.fontSubtitle)
            tintColor: enabled ? GTheme.textSecondary : GTheme.textDisabled
            enabled: folderSelector.enabled
            activeFocusOnTab: true
            Accessible.name: qsTr("Folder history")
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Folder history")
            KeyNavigation.tab: textField
            onClicked: historyPopup.opened ? historyPopup.close() : historyPopup.open()
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: GTheme.sizeLarge
            Layout.preferredHeight: GTheme.sizeLarge

            TextField {
                id: textField
                anchors.fill: parent
                readOnly: folderSelector.readOnly
                enabled: folderSelector.enabled
                text: folderSelector.path
                selectByMouse: true
                leftPadding: GTheme.spaceMD
                rightPadding: GTheme.spaceMD
                font.pixelSize: GTheme.fontBody
                color: readOnly && !activeFocus ? "transparent"
                                                   : (enabled ? GTheme.textPrimary : GTheme.textDisabled)
                selectionColor: GTheme.primaryLight(5)
                selectedTextColor: GTheme.textPrimary
                activeFocusOnTab: true
                Accessible.name: qsTr("Download folder")
                KeyNavigation.tab: selectorBtn
                KeyNavigation.backtab: historyBtn

                background: Rectangle {
                    color: textField.enabled ? GTheme.surfaceBase : GTheme.fillLighter
                    border.color: textField.activeFocus ? GTheme.focusRing : GTheme.borderBase
                    border.width: textField.activeFocus ? 2 : 1
                    radius: GTheme.radiusMedium

                    Behavior on border.color {
                        ColorAnimation { duration: GTheme.durationBase }
                    }
                }

                onEditingFinished: {
                    if (text !== folderSelector.path) {
                        let value = text
                        text = Qt.binding(function() { return folderSelector.path })
                        folderSelector.textChanged(value)
                    }
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        selectAll()
                        folderSelector.actived()
                    }
                }
            }

            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: GTheme.spaceMD
                anchors.rightMargin: GTheme.spaceMD
                visible: textField.readOnly && !textField.activeFocus
                enabled: false
                text: folderSelector.path
                font.pixelSize: GTheme.fontBody
                color: folderSelector.enabled ? GTheme.textPrimary : GTheme.textDisabled
                elide: Text.ElideMiddle
                maximumLineCount: 1
            }
        }

        GButton {
            id: selectorBtn
            Layout.preferredWidth: folderSelector.showBrowseLabel ? 104 : GTheme.sizeLarge
            Layout.preferredHeight: GTheme.sizeLarge
            Layout.minimumWidth: folderSelector.showBrowseLabel ? 88 : GTheme.sizeLarge
            size: "large"
            text: folderSelector.showBrowseLabel ? qsTr("Browse") : ""
            iconName: "folder"
            imageSize: Qt.size(GTheme.fontSubtitle, GTheme.fontSubtitle)
            tintColor: enabled ? GTheme.textSecondary : GTheme.textDisabled
            enabled: folderSelector.enabled
            activeFocusOnTab: true
            Accessible.name: qsTr("Browse folders")
            ToolTip.visible: hovered && !folderSelector.showBrowseLabel
            ToolTip.text: qsTr("Browse folders")
            KeyNavigation.tab: historyBtn
            KeyNavigation.backtab: textField
            onClicked: folderDialog.open()
        }
    }

    Popup {
        id: historyPopup
        x: 0
        y: folderSelector.height + GTheme.spaceXS
        width: folderSelector.width
        padding: 1
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: GTheme.surfaceElevated
            border.color: GTheme.borderLight
            border.width: 1
            radius: GTheme.radiusMedium
        }

        contentItem: ListView {
            id: historyList
            implicitHeight: Math.min(contentHeight, 240)
            model: folderSelector.historyModel
            clip: true

            delegate: ItemDelegate {
                width: historyList.width
                height: GTheme.sizeLarge
                hoverEnabled: true

                background: Rectangle {
                    color: parent.hovered ? GTheme.fillLight : "transparent"
                    radius: GTheme.radiusBase
                }

                contentItem: RowLayout {
                    spacing: GTheme.spaceSM

                    AuroraIcon {
                        Layout.preferredWidth: GTheme.fontSubtitle
                        Layout.preferredHeight: GTheme.fontSubtitle
                        name: "history"
                        iconSize: GTheme.fontSubtitle
                        color: GTheme.textSecondary
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: model.path
                        color: GTheme.textPrimary
                        elide: Text.ElideMiddle
                        maximumLineCount: 1
                        font.pixelSize: GTheme.fontBody
                    }

                    GButton {
                        Layout.preferredWidth: GTheme.sizeSmall
                        Layout.preferredHeight: GTheme.sizeSmall
                        visible: parent.parent.hovered || activeFocus
                        variant: "plain"
                        iconName: "delete"
                        imageSize: Qt.size(GTheme.fontCaption, GTheme.fontCaption)
                        tintColor: GTheme.dangerColor
                        Accessible.name: qsTr("Remove from history")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Remove from history")
                        onClicked: folderSelector.historyModel.removePath(index)
                    }
                }

                onClicked: {
                    folderSelector.path = model.path
                    historyPopup.close()
                    folderSelector.actived()
                    folderSelector.historySelected(model.path)
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }

    FolderDialog {
        id: folderDialog
        folder: Qt.resolvedUrl(folderSelector.path)

        onAccepted: {
            if (folder !== "") {
                let newPath = Utils.urlToLocalPath(folder)
                folderSelector.path = newPath
                folderSelector.historyModel.addPath(newPath)
            }
            folder = Qt.binding(function() { return Qt.resolvedUrl(folderSelector.path) })
            folderSelector.actived()
        }
    }
}
