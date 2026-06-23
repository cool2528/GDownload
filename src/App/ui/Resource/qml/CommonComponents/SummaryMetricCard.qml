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
    // 摘要指标卡用于容器内的指标条:无独立边框/阴影,避免与外层容器形成"卡中卡"双重边框。
    // 用 variant "default"(GCard 仅在 variant!="default" 时强制 1px 边框),配合 outlined:false
    // 实现真正无边框;仅呈现彩色气泡 + 数字 + 标签(对齐设计稿摘要条)
    outlined: false
    hoverEnabled: false
    shadow: false
    variant: "default"
    padding: GTheme.spaceSM
    radius: GTheme.radiusRound
    implicitHeight: GTheme.sizeLarge + GTheme.spaceSM * 2

    readonly property color accentColor: {
        switch (accent) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "info": return GTheme.infoColor
        default: return GTheme.primaryColor
        }
    }

    // 图标气泡底色:取对应 accent 的低饱和底(V5 彩色气泡),不再用白底+边框
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
        spacing: GTheme.spaceSM

        Rectangle {
            Layout.preferredWidth: GTheme.sizeDefault
            Layout.preferredHeight: GTheme.sizeDefault
            Layout.minimumWidth: GTheme.sizeDefault
            Layout.minimumHeight: GTheme.sizeDefault
            Layout.alignment: Qt.AlignVCenter
            radius: GTheme.radiusMedium
            color: root.accentBubble

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
