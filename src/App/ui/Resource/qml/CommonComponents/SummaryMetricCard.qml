import QtQuick
import QtQuick.Layouts
import gdl.sdk

// Aurora 紧凑指标卡：适用于首页和下载中心的 summary strip。
// 完整 Metric Card 在设计系统中最小高度为 112；生产页现有 summary strip 使用 70px 紧凑变体，
// 因此这里保留紧凑密度，并确保在四列最小宽度下图标、数值和单位不会互相遮挡。
GCard {
    id: root

    property string title: ""
    property string value: ""
    property string unit: ""
    property string detail: ""
    property bool expanded: false
    // iconSource 保留兼容；新调用应优先使用 Aurora 语义 iconName。
    property int iconSource: 0
    property string iconName: ""
    property string accent: "primary" // primary | success | warning | info

    compact: true
    interactive: false
    outlined: false
    hoverEnabled: false
    shadow: false
    variant: "muted"
    padding: expanded ? GTheme.spaceLG : GTheme.spaceSM
    radius: GTheme.radiusLarge
    implicitWidth: 132
    implicitHeight: expanded ? 124 : GTheme.sizeLarge + GTheme.spaceSM * 2
    Accessible.name: unit.length > 0
                     ? qsTr("%1: %2 %3").arg(title).arg(value).arg(unit)
                     : qsTr("%1: %2").arg(title).arg(value)

    background: Rectangle {
        radius: root.radius
        color: root.enabled ? GTheme.surfaceBase : GTheme.fillLighter
        border.width: 1
        border.color: GTheme.borderLighter
    }

    readonly property color accentColor: {
        switch (accent) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "info": return GTheme.infoColor
        default: return GTheme.primaryColor
        }
    }

    readonly property color accentBubble: {
        switch (accent) {
        case "success": return GTheme.bgSuccess
        case "warning": return GTheme.bgWarning
        case "info": return GTheme.bgInfo
        default: return GTheme.primaryLight(9)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: GTheme.spaceSM
        visible: !root.expanded

        Rectangle {
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.minimumWidth: GTheme.sizeDefault
            Layout.minimumHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignVCenter
            radius: GTheme.radiusMedium
            color: root.accentBubble

            AuroraIcon {
                anchors.centerIn: parent
                visible: root.iconName.length > 0
                name: root.iconName.length > 0 ? root.iconName : "info"
                iconSize: GTheme.fontBody
                color: root.enabled ? root.accentColor : GTheme.textDisabled
            }

            FontIcon {
                anchors.centerIn: parent
                visible: root.iconName.length === 0 && root.iconSource > 0
                iconSource: root.iconSource
                iconSize: GTheme.fontBody
                color: root.enabled ? root.accentColor : GTheme.textDisabled
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.value
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                    color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Text {
                    Layout.maximumWidth: 72
                    visible: root.unit.length > 0
                    text: root.unit
                    font.pixelSize: GTheme.fontCaption
                    color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    Layout.alignment: Qt.AlignBottom
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.title
                font.pixelSize: GTheme.fontCaption
                color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: GTheme.spaceSM
        visible: root.expanded

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceSM

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                radius: GTheme.radiusMedium
                color: root.accentBubble

                AuroraIcon {
                    anchors.centerIn: parent
                    visible: root.iconName.length > 0
                    name: root.iconName.length > 0 ? root.iconName : "info"
                    iconSize: GTheme.fontSubtitle
                    color: root.enabled ? root.accentColor : GTheme.textDisabled
                }

                FontIcon {
                    anchors.centerIn: parent
                    visible: root.iconName.length === 0 && root.iconSource > 0
                    iconSource: root.iconSource
                    iconSize: GTheme.fontSubtitle
                    color: root.enabled ? root.accentColor : GTheme.textDisabled
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.title
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightDemiBold
                color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceSM

            Text {
                text: root.value
                font.pixelSize: GTheme.fontH1
                font.weight: GTheme.weightDemiBold
                color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: root.unit
                visible: root.unit.length > 0
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightDemiBold
                color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
                elide: Text.ElideRight
                maximumLineCount: 1
                Layout.alignment: Qt.AlignBottom
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            visible: root.detail.length > 0
            text: root.detail
            font.pixelSize: GTheme.fontCaption
            color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }
}
