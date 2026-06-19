import QtQuick
import Qt5Compat.GraphicalEffects
import gdl.sdk

// 阴影注入共享组件:按 level(1-4)从 GTheme 读取阴影描述并注入 DropShadow
// 用法:GElevation { level: 2; content: someItem }
Item {
    id: root

    // 阴影层级 1-4,对应 GTheme.elevation1..4
    property int level: 1

    // 被包裹内容
    default property alias content: sourceLoader.sourceComponent

    // 当前层级阴影描述(随主题变化)
    readonly property var elevation: {
        switch (level) {
            case 1: return GTheme.elevation1
            case 2: return GTheme.elevation2
            case 3: return GTheme.elevation3
            case 4: return GTheme.elevation4
            default: return GTheme.elevation1
        }
    }

    Loader {
        id: sourceLoader
        anchors.fill: parent
    }

    DropShadow {
        anchors.fill: sourceLoader
        source: sourceLoader.item
        visible: sourceLoader.item !== null
        color: elevation.color ?? Qt.rgba(0, 0, 0, 0.1)
        radius: elevation.blur ?? 4
        horizontalOffset: elevation.offsetX ?? 0
        verticalOffset: elevation.offsetY ?? 1
        spread: elevation.spread ?? 0
        transparentBorder: true
    }
}
