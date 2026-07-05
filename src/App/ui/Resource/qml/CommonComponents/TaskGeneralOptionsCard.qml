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
    outlined: true
    padding: standardSpacing

    ColumnLayout {
        anchors.fill: parent
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
            columns: 4
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
                Layout.preferredHeight: 32
                placeholderText: qsTr("Optional filename")
            }

            Text {
                text: qsTr("Splits:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter
            }

            GSpinBox {
                id: spinbox
                Layout.preferredWidth: 100
                Layout.preferredHeight: 32
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
                Layout.preferredHeight: 32
                Layout.columnSpan: 3
                path: SettingsManager.qDir
            }
        }
    }
}
