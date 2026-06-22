import QtQuick
import QtQuick.Layouts
import gdl.sdk

// 下载中心快捷入口卡:URL / Torrent / Baidu,点击后复用现有添加任务入口
GCard {
    id: root

    property string title: ""
    property string description: ""
    property int iconSource: 0
    property string accent: "primary" // primary | success | warning | info
    signal clicked()

    compact: true
    interactive: true
    outlined: true
    hoverEnabled: true
    // hover 时复用 GCard selected 视觉反馈(加粗主色边框),弥补 accent 变体下
    // GCard 自身 hover 背景无变化的缺失,给可点击卡片以明确 hover 反馈
    selected: actionArea.containsMouse
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

            Text {
                text: root.title
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: root.description
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        id: actionArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
