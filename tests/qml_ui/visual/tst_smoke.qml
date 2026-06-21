import QtQuick
import QtTest

// Phase 2 smoke:验证 qml_ui_visual 可执行文件 + offscreen 平台 + QuickTest 框架可用
// Phase 3 起替换为 27 视觉用例 × 2 主题
TestCase {
    name: "smoke"
    // smoke 用例不依赖窗口渲染,设 true 立即执行避免 windowShown 等待
    when: true

    function test_pass() {
        compare(1 + 1, 2)
    }
}
