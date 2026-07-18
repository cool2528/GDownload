import QtQuick
import QtQuick.Controls
import gdl.sdk
ProgressBar {
    id: control
    padding:0
    // 兼容旧属性
    property color bkColor: GTheme.fillBase
    property color fgColor: GTheme.primaryColor
    // 规格化扩展
    // status: normal | success | warning | exception（danger 作为 exception 别名）
    property string status: "normal"
    property bool showText: false
    // 进度条类型：当前实现 line；后续可扩展 circle/dashboard
    property string progressType: "line"
    readonly property color resolvedBarColor: {
        switch (status) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "danger":
        case "exception": return GTheme.dangerColor
        default: return fgColor
        }
    }
    readonly property real progressRatio: to === from ? 0 : Math.max(0, Math.min(1, (value - from) / (to - from)))

    implicitHeight: showText ? GTheme.sizeSmall : 6
    background: Rectangle {
        implicitWidth: control.width
        implicitHeight: control.height
        color: bkColor
        radius: control.height / 2
    }

    contentItem: Item {
        implicitWidth: control.width
        implicitHeight: control.height

        Rectangle {
            width: control.visualPosition <= 0 ? 0 : Math.max(height, control.visualPosition * control.width)
            height: control.height
            radius: height / 2
            color: control.resolvedBarColor

            Behavior on width {
                NumberAnimation {
                    duration: GTheme.durationBase
                    easing.type: GTheme.easingStandard
                }
            }
        }

        // 进度文本（可选）
        Text {
            anchors.centerIn: parent
            visible: showText
            text: Math.round(control.progressRatio * 100) + "%"
            color: GTheme.textPrimary
            font.pixelSize: GTheme.fontCaption
        }
    }
}
