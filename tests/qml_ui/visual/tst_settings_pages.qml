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

    // Theme options previously used fixed 60 px image buttons beside a label,
    // which overflowed the usable settings width. Keep both responsive modes
    // covered so future visual changes cannot reintroduce horizontal overlap.
    function test_theme_picker_geometry() {
        var widths = [416, 320]
        var optionNames = ["systemThemeButton", "lightThemeButton", "darkThemeButton"]

        for (var w = 0; w < widths.length; ++w) {
            testCase.width = widths[w]
            harness.load("qrc:/qml/CommonComponents/ThemeSwitch.qml")
            wait(100)

            var picker = harness.loadedItem
            verify(picker !== null)

            for (var i = 0; i < optionNames.length; ++i) {
                var option = findChild(picker, optionNames[i])
                verify(option !== null, "Missing " + optionNames[i])
                var topLeft = option.mapToItem(picker, 0, 0)
                verify(topLeft.x >= -0.5, optionNames[i] + " starts outside the picker")
                verify(topLeft.x + option.width <= picker.width + 0.5,
                       optionNames[i] + " extends outside the picker at " + widths[w] + " px")
                verify(topLeft.y >= -0.5, optionNames[i] + " starts above the picker")
                verify(topLeft.y + option.height <= picker.implicitHeight + 0.5,
                       optionNames[i] + " extends below implicitHeight at " + widths[w] + " px")
            }
        }
    }

    function verifyHorizontallyContained(item, root, label) {
        verify(item !== null, "Missing " + label)
        var topLeft = item.mapToItem(root, 0, 0)
        verify(topLeft.x >= -0.5,
               label + " starts outside its page at x=" + topLeft.x)
        verify(topLeft.x + item.width <= root.width + 0.5,
               label + " extends outside its page: right=" +
               (topLeft.x + item.width) + ", page=" + root.width)
    }

    // Browser Extension previously used fixed 100/140/160 px action rows and
    // Segoe glyphs. Capture the actual narrow page in both themes and assert
    // that every primary action remains inside the scroll content width.
    function test_lab_narrow_light_dark() {
        var narrowThemes = ["light", "dark"]
        var actionNames = [
            "extensionChromeButton",
            "extensionFirefoxButton",
            "extensionEdgeButton",
            "extensionViewConfigurationButton",
            "extensionCopyUrlButton",
            "extensionCopySecretButton",
            "extensionCopyAllButton",
            "extensionIssuesButton",
            "extensionDocsButton",
            "extensionWebsiteButton",
            "extensionStarButton"
        ]

        testCase.width = 420
        testCase.height = 720

        for (var t = 0; t < narrowThemes.length; ++t) {
            harness.themeMode = narrowThemes[t]
            harness.load("qrc:/qml/Browser/LabSettingPage.qml")
            wait(250)

            var page = harness.loadedItem
            verify(page !== null)
            for (var i = 0; i < actionNames.length; ++i)
                verifyHorizontallyContained(findChild(page, actionNames[i]), page, actionNames[i])

            var tag = "lab_narrow_" + narrowThemes[t]
            verify(Screenshot.capture(harness, tag, harness.themeMode),
                   "Screenshot.capture failed for " + tag)
        }
    }

    // The preferences shell switches from a fixed sidebar to a horizontal
    // navigation rail. Keep the three semantic-icon buttons bounded at the
    // narrow breakpoint in Light and Dark themes.
    function test_settings_shell_narrow_light_dark() {
        var narrowThemes = ["light", "dark"]
        var navNames = ["settingsBasicNav", "settingsAdvancedNav", "settingsLabNav"]

        testCase.width = 560
        testCase.height = 720

        for (var t = 0; t < narrowThemes.length; ++t) {
            harness.themeMode = narrowThemes[t]
            harness.load("qrc:/qml/Browser/SettingsPageView.qml")
            wait(250)

            var shell = harness.loadedItem
            verify(shell !== null)
            for (var i = 0; i < navNames.length; ++i)
                verifyHorizontallyContained(findChild(shell, navNames[i]), shell, navNames[i])

            var tag = "settings_index_narrow_" + narrowThemes[t]
            verify(Screenshot.capture(harness, tag, harness.themeMode),
                   "Screenshot.capture failed for " + tag)
        }
    }

    // Long User-Agent values must remain inside the card at the same narrow
    // width used by the compact Preferences shell.
    function test_advanced_long_values_narrow_light_dark() {
        var narrowThemes = ["light", "dark"]
        var cases = [
            {
                tag: "useragent_long_narrow",
                path: "qrc:/qml/Browser/UserAgentSettingPage.qml",
                prepare: function(page) {
                    var field = findChild(page, "customUserAgentField")
                    verify(field !== null)
                    field.text = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 " +
                                 "(KHTML, like Gecko) Chrome/999.0.0.0 Safari/537.36 " +
                                 "AuroraCompatibilityValidation/2026.07"
                    verifyHorizontallyContained(field, page, "custom User-Agent field")
                }
            }
        ]

        testCase.width = 420
        for (var c = 0; c < cases.length; ++c) {
            for (var t = 0; t < narrowThemes.length; ++t) {
                testCase.height = 900
                harness.themeMode = narrowThemes[t]
                harness.load(cases[c].path)
                wait(200)

                var page = harness.loadedItem
                verify(page !== null)
                cases[c].prepare(page)
                wait(100)

                testCase.height = Math.ceil(page.implicitHeight)
                wait(50)
                var tag = cases[c].tag + "_" + narrowThemes[t]
                verify(Screenshot.capture(harness, tag, harness.themeMode),
                       "Screenshot.capture failed for " + tag)
            }
        }
    }

    // Capture the cards below the Lab viewport as standalone narrow surfaces,
    // so installation, long connection values, FAQ wrapping, and support
    // actions receive direct Light/Dark review coverage.
    function test_extension_cards_narrow_light_dark() {
        var cards = [
            {
                tag: "extension_installation_narrow",
                path: "qrc:/qml/Browser/BrowserExtension/InstallationGuideCard.qml"
            },
            {
                tag: "extension_configuration_narrow",
                path: "qrc:/qml/Browser/BrowserExtension/ConfigHelperCard.qml"
            },
            {
                tag: "extension_faq_narrow",
                path: "qrc:/qml/Browser/BrowserExtension/FAQCard.qml"
            }
        ]
        var narrowThemes = ["light", "dark"]

        testCase.width = 420
        for (var c = 0; c < cards.length; ++c) {
            testCase.height = 720
            for (var t = 0; t < narrowThemes.length; ++t) {
                harness.themeMode = narrowThemes[t]
                harness.load(cards[c].path)
                wait(250)

                var card = harness.loadedItem
                verify(card !== null)
                testCase.height = Math.ceil(card.implicitHeight)
                wait(100)
                compare(Math.ceil(card.height), testCase.height)

                var tag = cards[c].tag + "_" + narrowThemes[t]
                verify(Screenshot.capture(harness, tag, harness.themeMode),
                       "Screenshot.capture failed for " + tag)
            }
        }
    }

    function test_extension_faq_expanded_narrow_light_dark() {
        var narrowThemes = ["light", "dark"]
        testCase.width = 420

        for (var t = 0; t < narrowThemes.length; ++t) {
            testCase.height = 720
            harness.themeMode = narrowThemes[t]
            harness.load("qrc:/qml/Browser/BrowserExtension/FAQCard.qml")
            wait(200)

            var card = harness.loadedItem
            verify(card !== null)
            var connectionQuestion = findChild(card, "extensionFaqConnection")
            var protectedQuestion = findChild(card, "extensionFaqProtectedSites")
            verify(connectionQuestion !== null)
            verify(protectedQuestion !== null)
            connectionQuestion.expanded = true
            protectedQuestion.expanded = true
            wait(100)

            testCase.height = Math.ceil(card.implicitHeight)
            wait(100)
            verifyHorizontallyContained(connectionQuestion, card, "expanded connection FAQ")
            verifyHorizontallyContained(protectedQuestion, card, "expanded protected-site FAQ")

            var tag = "extension_faq_expanded_narrow_" + narrowThemes[t]
            verify(Screenshot.capture(harness, tag, harness.themeMode),
                   "Screenshot.capture failed for " + tag)
        }
    }

}
