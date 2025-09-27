import QtQuick
import QtQuick.Controls
import gdl.sdk
ProgressBar {
    id: control
    padding:0
    // 兼容旧属性
    property color bkColor: GTheme.fillLight
    property color fgColor: GTheme.primaryColor
    // 规格化扩展
    // status: normal | success | warning | exception
    property string status: "normal"
    property bool showText: false
    // 进度条类型：当前实现 line；后续可扩展 circle/dashboard
    property string progressType: "line"
    readonly property color resolvedBarColor: (function(){
        if (fgColor) return fgColor;
        switch(status){
            case "success": return GTheme.successColor;
            case "warning": return GTheme.warningColor;
            case "exception": return GTheme.dangerColor;
            default: return GTheme.primaryColor;
        }
    })()
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
            width: control.visualPosition * control.width
            height: control.height
            radius: height / 2
            color: resolvedBarColor
        }

        // 进度文本（可选）
        Text {
            anchors.centerIn: parent
            visible: showText
            text: Math.round((control.value - control.from) / (control.to - control.from) * 100) + "%"
            color: GTheme.textSecondary
            font.pixelSize: 12
        }
    }
}
