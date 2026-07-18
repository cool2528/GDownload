import QtQuick
import gdl.sdk 1.0

// 单页加载外壳
// 视觉用例通过 harness.load("qrc:/qml/...") 加载目标页面,
// 通过 harness.themeMode 切换 "light"/"dark",onThemeModeChanged 调用
// GTheme.Settheme(GThemeType.ThemeMode.Light/Dark) 同步主题到桩单例,
// 桩单例随后 emit darkChanged() 触发页面所有 GTheme.X 绑定刷新。
// 必须显式 `import gdl.sdk 1.0` 才能引用 GTheme 单例与 GThemeType 枚举,
// 否则 QML 引擎报 ReferenceError: GTheme is not defined。
Rectangle {
    id: harness
    color: GTheme.bgPage
    property string themeMode: "light"
    property alias source: loader.source
    // 已加载项引用(Loader.item):供视觉用例对 Popup/Dialog 类组件调用 open()
    // 等方法。Loader 未加载时为 null。
    property alias loadedItem: loader.item
    function load(url) { loader.source = url }
    function loadWithProperties(url, properties) { loader.setSource(url, properties) }

    // 主题切换:GThemeType 枚举值 kSystem=0 kLight=1 kDark=2
    // 枚举名沿用 C++ enum class 原名(带 k 前缀),与 ThemeSwitch.qml 调用一致。
    // GTheme.theme 是 QML_AUTO_PROPERTY 生成的属性,QML 端可写
    onThemeModeChanged: {
        if (themeMode === "dark") {
            GTheme.theme = GThemeType.ThemeMode.kDark
        } else if (themeMode === "light") {
            GTheme.theme = GThemeType.ThemeMode.kLight
        } else {
            GTheme.theme = GThemeType.ThemeMode.kSystem
        }
    }

    // 初始主题同步:Component.onCompleted 触发一次主题应用
    Component.onCompleted: {
        if (themeMode === "dark") {
            GTheme.theme = GThemeType.ThemeMode.kDark
        } else {
            GTheme.theme = GThemeType.ThemeMode.kLight
        }
    }

    Loader {
        id: loader
        anchors.fill: parent
        asynchronous: false
        // 被加载页面根 Item 多数未声明 anchors.fill: parent(如 DownloadPageView
        // 根是 Item,仅内部 RowLayout anchors.fill: parent),默认尺寸 0x0 导致
        // grabToImage 截图为空白。这里在 onLoaded 把加载项尺寸绑定到 Loader,
        // 使页面铺满 harness,截图才有内容。
        onLoaded: {
            item.width = Qt.binding(() => loader.width)
            item.height = Qt.binding(() => loader.height)
        }
    }
}
