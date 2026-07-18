import QtQuick

// 历史 numeric iconSource 兼容层。新代码应直接传 iconName；已知旧码点
// 映射到 Aurora SVG，未知码点安全地不渲染，不再加载 Segoe 字体。
Item {
    id: control
    property string iconName: ""
    property int iconSource: 0
    property int iconSize: 16
    property color color: GTheme.textPrimary
    property int horizontalAlignment: Text.AlignHCenter
    property int verticalAlignment: Text.AlignVCenter
    readonly property string resolvedIconName: iconName.length > 0
                                               ? iconName
                                               : legacySemanticName(iconSource)

    function legacySemanticName(value) {
        switch (value) {
        case 0xe930: return "completed"
        case 0xea39: return "error-badge"
        case 0xe897: return "help"
        case 0xe946: return "info"
        case 0xe7ba: return "warning"
        case 0xe711: return "close"
        case 0xe713: return "settings"
        case 0xea80: return "lightbulb"
        case 0xe943: return "repository"
        case 0xe774: return "globe"
        case 0xe70d: return "chevron-down"
        case 0xe76c: return "chevron-right"
        case 0xe71b: return "link"
        case 0xe945: return "lightning"
        case 0xe790: return "palette"
        case 0xe72e: return "lock"
        case 0xe71c: return "filter"
        default: return ""
        }
    }

    // 设置默认尺寸
    implicitWidth: iconSize
    implicitHeight: iconSize

    AuroraIcon {
        anchors.centerIn: parent
        visible: control.resolvedIconName.length > 0
        name: control.resolvedIconName.length > 0 ? control.resolvedIconName : "info"
        iconSize: control.iconSize
        color: control.color
    }
}
