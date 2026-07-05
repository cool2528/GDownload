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
    // fgColor 为 color 属性恒有效,原 if(fgColor)+switch 分支不可达(死逻辑),直接取 fgColor(T5)
    readonly property color resolvedBarColor: fgColor
    // 渐变末端色:正常/成功态走 primary→success 的消费级渐变(V5),
    // 警告/异常态用同色系自身的浅色起点,保持语义色不混入绿色
    readonly property color gradStart: (status === "warning" || status === "exception")
                                       ? Qt.lighter(resolvedBarColor, 1.25) : GTheme.primaryColor
    readonly property color gradEnd: (status === "warning" || status === "exception")
                                     ? resolvedBarColor : GTheme.successColor
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
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: control.gradStart }
                GradientStop { position: 1.0; color: control.gradEnd }
            }
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
