import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

// 图片按钮:双图(normal/hover)切换 + 可选 tintColor 着色
// 语义聚焦标题栏窗口控件等"图片管线"场景,与 GButton(字体图标/文字)分工
Button {
    id: control
    property url normalImage
    property url hoverImage
    property color backgroundColor: "transparent"
    // 可选:为图标着色(暗色主题下保证对比)
    property color tintColor: "transparent"
    property size imageSize: Qt.size(width, height)

    // 令牌化默认尺寸兜底;调用方通常用 Layout.preferred* 覆盖
    implicitWidth: GTheme.sizeDefault
    implicitHeight: GTheme.sizeDefault

    background: Rectangle {
        color: control.backgroundColor
        radius: GTheme.radiusSmall
    }

    Image {
        id: image
        source: mouse.hovered ? control.hoverImage : control.normalImage
        anchors.centerIn: parent
        width: control.imageSize.width
        height: control.imageSize.height
        mipmap: true
        smooth: true
        fillMode: Image.PreserveAspectFit
        layer.enabled: !Qt.colorEqual(control.tintColor, "transparent")
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: control.tintColor
        }
    }

    HoverHandler {
        id: mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
