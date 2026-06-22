import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

Item {
    id: headerRoot

    property string title: qsTr("Add New Download Task")
    property string subtitle: qsTr("Add downloads from URLs, torrents, or cloud storage")
    signal closeRequested()

    // 页面级布局常量:TaskDialog 头部高度与图标块尺寸,非通用设计令牌
    readonly property int headerHeight: GTheme.titleBarHeight + GTheme.space2XL
    readonly property int iconBoxSize: GTheme.sizeLarge
    readonly property int closeButtonSize: GTheme.sizeSmall + GTheme.spaceXS

    Layout.fillWidth: true
    Layout.preferredHeight: headerHeight
    implicitHeight: headerHeight

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: GTheme.space2XL
        anchors.rightMargin: GTheme.space2XL
        spacing: GTheme.spaceMD

        Rectangle {
            Layout.preferredWidth: headerRoot.iconBoxSize
            Layout.preferredHeight: headerRoot.iconBoxSize
            Layout.alignment: Qt.AlignVCenter
            color: GTheme.primaryLight(9)
            radius: GTheme.radiusBase

            FontIcon {
                anchors.centerIn: parent
                iconSource: SegoeFluentIcons.Add
                iconSize: GTheme.fontTitle
                color: GTheme.primaryColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            Text {
                text: headerRoot.title
                font.pixelSize: GTheme.fontTitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: headerRoot.subtitle
                font.pixelSize: GTheme.fontBody
                color: GTheme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        GButton {
            iconSource: SegoeFluentIcons.ChromeClose
            iconSize: GTheme.fontBody
            Layout.preferredWidth: headerRoot.closeButtonSize
            Layout.preferredHeight: headerRoot.closeButtonSize
            onClicked: headerRoot.closeRequested()
        }
    }
}
