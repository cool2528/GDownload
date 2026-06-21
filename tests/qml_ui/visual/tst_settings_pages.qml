import QtQuick
import QtTest
import "qrc:/tests/qml_ui/support"

// 视觉用例:14 个设置页 × 2 主题 = 28 张截图
//
// 覆盖范围(Task 7):
//   - BasicSettingPage                 基础设置(主题/语言/行为/路径/代理/剪贴板)
//   - AdvancedSettingPage              高级设置(容器:索引页,内嵌多个子页)
//   - Aria2RpcSettingPage              RPC 端口与密钥
//   - BaiduCookieSettingPage           百度网盘 Cookie
//   - BitTorrentAdvancedSettingPage    BT 高级(DHT/最大 peer/加密)
//   - ConnectionPerformanceSettingPage 连接性能(并发/分片/单服务器连接数)
//   - LabSettingPage                   实验室页(浏览器插件功能亮点卡片)
//   - PostDownloadActionsSettingPage   下载完成后动作
//   - SpeedControlSettingPage          全局上下行限速
//   - TimeoutRetrySettingPage          超时与重试
//   - TrackerServerSettingPage         Tracker 服务器源
//   - UserAgentSettingPage             User-Agent
//   - SettingsPageView                 设置索引容器(含侧导航 + StackLayout)
//   - SettingPageTitle                 设置页标题组件
//
// 数据驱动:单 test_all_pages 函数遍历 pages × themes,生成 28 张截图。
// 仅 1 个 ctest 用例,视觉覆盖率优先于单页隔离。
//
// 依赖:Task 6 已注册 GTheme / BrowserManager / SettingsManager / ToastManager /
// NetWorkDiskManager / UpdateManager / UtilsToolsManager / LanguageManager /
// FolderHistoryModel / GThemeType / SegoeFluentIcons / FluentIcons / Basic 样式。
// Task 7 扩展了 TestSettingsManager(33 属性 + 35 setter)、TestLanguageManager
// (3 方法)、TestUtilsToolsManager(SetAutoStart/RelaunchAfterExit/serverList)、
// TestBrowserManager(SyncTrackersServerlist + sigTrackerUpdateStatus 信号),
// 使 14 个设置页可加载、可渲染。
TestCase {
    id: testCase
    name: "tst_settings_pages"
    when: windowShown
    width: 900
    height: 600

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    // 14 页 × 2 主题:tag 为截图文件名(无后缀),path 为 qrc 内 QML 路径
    // tag 命名:语义短名 + _light/_dark,与 manifest.jsonl 的 page 字段一致
    property var pages: [
        { tag: "basic",               path: "qrc:/qml/Browser/BasicSettingPage.qml" },
        { tag: "advanced",            path: "qrc:/qml/Browser/AdvancedSettingPage.qml" },
        { tag: "aria2rpc",            path: "qrc:/qml/Browser/Aria2RpcSettingPage.qml" },
        { tag: "baiducookie",         path: "qrc:/qml/Browser/BaiduCookieSettingPage.qml" },
        { tag: "bittorrentadvanced",  path: "qrc:/qml/Browser/BitTorrentAdvancedSettingPage.qml" },
        { tag: "connectionperf",      path: "qrc:/qml/Browser/ConnectionPerformanceSettingPage.qml" },
        { tag: "lab",                 path: "qrc:/qml/Browser/LabSettingPage.qml" },
        { tag: "postdownload",        path: "qrc:/qml/Browser/PostDownloadActionsSettingPage.qml" },
        { tag: "speedcontrol",        path: "qrc:/qml/Browser/SpeedControlSettingPage.qml" },
        { tag: "timeoutretry",        path: "qrc:/qml/Browser/TimeoutRetrySettingPage.qml" },
        { tag: "trackerserver",       path: "qrc:/qml/Browser/TrackerServerSettingPage.qml" },
        { tag: "useragent",           path: "qrc:/qml/Browser/UserAgentSettingPage.qml" },
        { tag: "settings_index",      path: "qrc:/qml/Browser/SettingsPageView.qml" },
        { tag: "page_title",          path: "qrc:/qml/Browser/SettingPageTitle.qml" }
    ]
    property var themes: ["light", "dark"]

    function init() {
        // 每用例前重置主题与默认尺寸
        harness.themeMode = "light"
        testCase.width = 900
        testCase.height = 600
        harness.load("")
        wait(20)
    }

    // 数据驱动:遍历 14 页 × 2 主题,产出 28 张截图
    function test_all_pages() {
        for (var i = 0; i < pages.length; ++i) {
            for (var t = 0; t < themes.length; ++t) {
                harness.themeMode = themes[t]
                harness.load(pages[i].path)
                // Loader 异步加载 + GTheme 主题传播 + 场景图渲染,留 250ms 余量
                wait(250)
                var tag = pages[i].tag + "_" + themes[t]
                var ok = Screenshot.capture(harness, tag, harness.themeMode)
                verify(ok, "Screenshot.capture failed for " + tag)
            }
        }
    }
}
