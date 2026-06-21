import QtQuick
import QtTest

// ScreenshotHelper 验证用例:捕获一个红色 Rectangle 并验证返回值
// 产物:build/test_artifacts/qml_ui/tst_screenshot_helper/red_square.png + manifest.jsonl 行
// when: windowShown 确保 TestCase 在渲染窗口中(grabToImage 需要场景图初始化)
TestCase {
    name: "tst_screenshot_helper"
    when: windowShown
    width: 200
    height: 200

    function test_capture_red_square() {
        // 创建被截图的 Rectangle(parent 设为 TestCase,确保进入场景图)
        var rect = Qt.createQmlObject(
            'import QtQuick; Rectangle { width: 100; height: 100; color: "red" }',
            this, "probe_rect")
        verify(rect, "failed to create probe Rectangle")
        try {
            // 等待场景图渲染一帧(offscreen 平台仍需事件循环处理 expose)
            wait(100)
            var ok = Screenshot.capture(rect, "red_square", "light")
            verify(ok, "Screenshot.capture() returned false (grab timeout or write failure)")
        } finally {
            rect.destroy()
        }
    }
}
