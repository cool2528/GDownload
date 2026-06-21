import QtQuick
import QtTest
import "qrc:/tests/qml_ui/support"

// 视觉用例:4 个 Lab 子卡片 × 2 主题 = 8 张截图(Task 8)
//
// 覆盖范围(spec Section 6.3, qrc:/qml/Browser/BrowserExtension/):
//   - FeatureHighlightCard   功能亮点(2x3 功能网格 + 装饰色图标)
//   - ConfigHelperCard       配置助手(WebSocket URL / RPC Secret + 复制按钮 + 状态指示器)
//   - FAQCard                常见问题(可展开折叠面板)
//   - InstallationGuideCard  安装指引(分步骤浏览器插件安装说明)
//
// 这些卡片根类型为 GCard(Control 子类,非 Popup),经 PageHarness.Loader 加载后
// item.width/height 绑定到 Loader 尺寸,grabToImage(harness) 可正常捕获内容。
//
// 数据驱动:单 test_all_cards 函数遍历 cards × themes,生成 8 张截图。
TestCase {
    id: testCase
    name: "tst_lab_subcards"
    when: windowShown
    width: 720
    height: 600

    PageHarness {
        id: harness
        anchors.fill: parent
        objectName: "harness"
    }

    // 4 卡片 × 2 主题:tag 为截图文件名(无后缀),path 为 qrc 内 QML 路径
    property var cards: [
        { tag: "feature_highlight", path: "qrc:/qml/Browser/BrowserExtension/FeatureHighlightCard.qml" },
        { tag: "config_helper",     path: "qrc:/qml/Browser/BrowserExtension/ConfigHelperCard.qml" },
        { tag: "faq",               path: "qrc:/qml/Browser/BrowserExtension/FAQCard.qml" },
        { tag: "installation_guide", path: "qrc:/qml/Browser/BrowserExtension/InstallationGuideCard.qml" }
    ]
    property var themes: ["light", "dark"]

    function init() {
        harness.themeMode = "light"
        testCase.width = 720
        testCase.height = 600
        harness.load("")
        wait(20)
    }

    // 数据驱动:遍历 4 卡片 × 2 主题,产出 8 张截图
    function test_all_cards() {
        for (var i = 0; i < cards.length; ++i) {
            for (var t = 0; t < themes.length; ++t) {
                harness.themeMode = themes[t]
                harness.load(cards[i].path)
                // Loader 异步加载 + GTheme 主题传播 + 场景图渲染,留 250ms 余量
                wait(250)
                var tag = cards[i].tag + "_" + themes[t]
                var ok = Screenshot.capture(harness, tag, harness.themeMode)
                verify(ok, "Screenshot.capture failed for " + tag)
            }
        }
    }
}
