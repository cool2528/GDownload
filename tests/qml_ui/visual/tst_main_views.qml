import QtQuick
import QtTest
import "qrc:/tests/qml_ui/support"

// 视觉用例:4 主页面 × 2 主题 + 2 全窗 = 10 张截图
//
// 覆盖范围:
//   - DownloadPageView  下载页主视图(左侧导航 + 列表堆栈)
//   - NetDiskPageView   网盘解析页(URL 输入 + 文件列表)
//   - NavigatorView     左侧导航栏(图标按钮组)
//   - mainWindow        主窗口(TitleBar + SplitView + Tray + Dialogs)
//   - full_window       DownloadPageView 在 1920x1080 全窗尺寸下的布局
//
// 主题切换:PageHarness.themeMode = "light"/"dark" 触发 GTheme.theme 写入,
// 桩 GTheme emit darkChanged 后页面所有 GTheme.X 绑定刷新。
//
// 桩单例:由 visual/main.cpp TestSetup 注册(TestGTheme / TestBrowserManager 等),
// 数据相关方法返回 nullptr 或空,页面以空状态渲染。
//
// mainWindow 依赖 FramelessHelper QML 模块;若测试环境未注册,Loader 失败,
// 截图为空白 harness,不影响其他用例产出。
TestCase {
    id: testCase
    name: "tst_main_views"
    when: windowShown
    width: 1280
    height: 720

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    function init() {
        // 每个用例前重置主题与默认尺寸
        harness.themeMode = "light"
        testCase.width = 1280
        testCase.height = 720
        harness.load("")
        wait(20)
    }

    // 通用截图辅助:加载 + 等待渲染 + 截图
    function loadAndCapture(url, tag, theme) {
        if (theme !== undefined) {
            harness.themeMode = theme
        }
        harness.load(url)
        // Loader 异步加载 + GTheme 主题传播 + 场景图渲染,留 250ms 余量
        wait(250)
        var ok = Screenshot.capture(harness, tag, harness.themeMode)
        verify(ok, "Screenshot.capture failed for " + tag)
    }

    // ---- DownloadPageView ----
    function test_download_light() {
        loadAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "download_light", "light")
    }
    function test_download_dark() {
        loadAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "download_dark", "dark")
    }

    // ---- HomePage (Browser/HomePage.qml) ----
    function test_home_light() {
        loadAndCapture("qrc:/qml/Browser/HomePage.qml", "home_light", "light")
    }
    function test_home_dark() {
        loadAndCapture("qrc:/qml/Browser/HomePage.qml", "home_dark", "dark")
    }

    // ---- NetDiskPageView (CommonComponents/NetDiskPageView.qml) ----
    function test_netdisk_light() {
        loadAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_light", "light")
    }
    function test_netdisk_dark() {
        loadAndCapture("qrc:/qml/CommonComponents/NetDiskPageView.qml", "netdisk_dark", "dark")
    }

    // ---- NavigatorView (Navigator/NavigatorView.qml) ----
    function test_navigator_light() {
        loadAndCapture("qrc:/qml/Navigator/NavigatorView.qml", "navigator_light", "light")
    }
    function test_navigator_dark() {
        loadAndCapture("qrc:/qml/Navigator/NavigatorView.qml", "navigator_dark", "dark")
    }

    // ---- mainWindow (顶层主窗口) ----
    // 依赖 FramelessHelper QML 模块;若未注册,Loader.item 为 null,截图仍会产出
    function test_mainwindow_light() {
        loadAndCapture("qrc:/qml/mainWindow.qml", "mainwindow_light", "light")
    }
    function test_mainwindow_dark() {
        loadAndCapture("qrc:/qml/mainWindow.qml", "mainwindow_dark", "dark")
    }

    // ---- full window (1920x1080 DownloadPageView) ----
    function test_full_window_light() {
        testCase.width = 1920
        testCase.height = 1080
        wait(120)  // 等待窗口尺寸应用 + 场景图重排
        loadAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "full_window_light", "light")
    }
    function test_full_window_dark() {
        testCase.width = 1920
        testCase.height = 1080
        wait(120)
        loadAndCapture("qrc:/qml/Browser/DownloadPageView.qml", "full_window_dark", "dark")
    }
}
