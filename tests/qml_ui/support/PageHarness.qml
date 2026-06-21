import QtQuick

// 单页加载外壳
// 视觉用例通过 harness.load("qrc:/qml/...") 加载目标页面,
// 通过 harness.themeMode 切换 "light"/"dark"(Phase 3 接入 GTheme.setTheme)
Item {
    id: harness
    property string themeMode: "light"
    property alias source: loader.source
    function load(url) { loader.source = url }
    Loader {
        id: loader
        anchors.fill: parent
    }
}
