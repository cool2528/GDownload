import QtQuick

// 设计令牌（只读常量集合）
// 说明：当前作为普通 QML 对象提供，使用时在页面内创建一次实例：
//   import "../CommonComponents"
//   DesignTokens { id: Tokens }
// 后续如需全局单例可改为 QML Singleton（需 qmldir）。
QtObject {
    id: tokens

    // 间距（px）
    readonly property int spacing4: 4
    readonly property int spacing8: 8
    readonly property int spacing12: 12
    readonly property int spacing16: 16
    readonly property int spacing20: 20
    readonly property int spacing24: 24

    // 圆角（px）
    readonly property int radius2: 2
    readonly property int radius4: 4
    readonly property int radius8: 8

    // 内边距（px）
    readonly property int paddingSmall: 8
    readonly property int paddingMedium: 12
    readonly property int paddingLarge: 16

    // 阴影参数（后续可与 QtQuick.Effects.MultiEffect 联动）
    readonly property real shadowBlurSmall: 8
    readonly property real shadowBlurMedium: 12
    readonly property real shadowBlurLarge: 18
    readonly property real shadowOffsetX: 0
    readonly property real shadowOffsetYSmall: 2
    readonly property real shadowOffsetYMedium: 4
    readonly property real shadowAlpha: 0.25
}

