import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

Item {
    id: headerRoot
    required property int headerHeight
    required property int standardPadding
    required property int standardSpacing
    property string title: qsTr("Add New Download Task")
    property string subtitle: qsTr("Add downloads from URLs, torrents, or cloud storage")
    signal closeRequested()

    Layout.fillWidth: true
    Layout.preferredHeight: headerHeight
    implicitHeight: headerHeight

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: standardPadding
            anchors.rightMargin: standardPadding
            spacing: standardSpacing

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignVCenter
                color: GTheme.primaryLight(9)
                radius: 8

                FontIcon {
                    anchors.centerIn: parent
                    iconSource: SegoeFluentIcons.Add
                    iconSize: 20
                    color: GTheme.primaryColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    id: titleLabel
                    text: headerRoot.title
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    color: GTheme.textPrimary
                }

                Text {
                    text: headerRoot.subtitle
                    font.pixelSize: 14
                    color: GTheme.textSecondary
                }
            }

            IconButton {
                iconSource: SegoeFluentIcons.ChromeClose
                iconSize: 14
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                iconColor: hovered ? GTheme.textPrimary : GTheme.textSecondary
                backgroundColor: hovered ? GTheme.fillLight : "transparent"
                onClicked: headerRoot.closeRequested()
            }
        }
    }
}
