import QtQuick
import QtQuick.Layouts
import gdl.sdk

// 下载中心摘要指标卡:用于速度、任务数、密度等横向 summary strip
GCard {
    id: root

    property string title: ""
    property string value: ""
    property string unit: ""
    property int iconSource: 0
    property string accent: "primary" // primary | success | warning | info

    compact: true
    outlined: true
    hoverEnabled: false
    variant: {
        switch (accent) {
        case "success": return "accentSuccess"
        case "warning": return "accentWarning"
        case "info": return "accentInfo"
        default: return "accentPrimary"
        }
    }
    padding: GTheme.spaceSM
    radius: GTheme.radiusBase
    implicitHeight: GTheme.sizeLarge + GTheme.spaceSM * 2

    readonly property color accentColor: {
        switch (accent) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "info": return GTheme.infoColor
        default: return GTheme.primaryColor
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: GTheme.spaceSM

        Rectangle {
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignVCenter
            radius: GTheme.radiusBase
            color: GTheme.bgWhite
            border.width: 1
            border.color: GTheme.borderLight

            FontIcon {
                anchors.centerIn: parent
                iconSource: root.iconSource
                iconSize: GTheme.fontBody
                color: root.accentColor
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS

                Text {
                    text: root.value
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    elide: Text.ElideRight
                }

                Text {
                    visible: root.unit.length > 0
                    text: root.unit
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    Layout.alignment: Qt.AlignBottom
                }
            }

            Text {
                text: root.title
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }
}
