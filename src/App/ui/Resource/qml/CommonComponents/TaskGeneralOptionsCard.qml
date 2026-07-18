import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

GCard {
    id: generalCard
    required property int standardSpacing
    property alias renameField: renameEdit
    property alias renameText: renameEdit.text
    property alias splitsSpinner: spinbox
    property alias splitsValue: spinbox.value
    property alias pathSelector: savePath
    property alias saveDirectory: savePath.path
    readonly property bool compactLayout: width < 520
    outlined: true
    padding: standardSpacing

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: generalCard.padding
        spacing: standardSpacing

        Text {
            text: qsTr("Download Settings")
            font.pixelSize: 14
            font.weight: Font.Medium
            color: GTheme.textPrimary
            Layout.alignment: Qt.AlignVCenter
            Layout.topMargin: 5
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            columns: generalCard.compactLayout ? 1 : 4
            columnSpacing: standardSpacing
            rowSpacing: 12

            Text {
                text: qsTr("Rename:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter
            }

            GTextField {
                id: renameEdit
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                placeholderText: qsTr("Optional filename")
                Accessible.name: qsTr("Filename")
            }

            Text {
                text: qsTr("Splits:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter
            }

            GSpinBox {
                id: spinbox
                Layout.fillWidth: generalCard.compactLayout
                Layout.preferredWidth: generalCard.compactLayout ? -1 : 100
                Layout.preferredHeight: GTheme.sizeLarge
                Accessible.name: qsTr("Connection splits")
                from: 1
                to: 64
                value: 64
            }

            Text {
                text: qsTr("Save to:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter
            }

            FolderSelector {
                id: savePath
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.columnSpan: generalCard.compactLayout ? 1 : 3
                path: SettingsManager.qDir
            }
        }
    }
}
