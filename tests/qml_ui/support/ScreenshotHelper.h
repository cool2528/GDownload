#pragma once

#include <QObject>
#include <QQuickItem>
#include <QString>

namespace gdl {
namespace tests {

// 测试期截图辅助
// QML 端通过 Screenshot.capture(item, "tag") 或 Screenshot.capture(item, "tag", "light") 调用
// 流程:
//   1. item->grabToImage() 异步截图(QEventLoop 阻塞等待 ready 信号,5s 超时)
//   2. PNG 写入 $QML_UI_ARTIFACT_DIR/<testName>/<tag>.png
//   3. 同步 append 一行 JSON 到 $QML_UI_ARTIFACT_DIR/manifest.jsonl
//      字段:test(TestCase 名) / case(tag) / page(tag) / theme / file(相对路径) / md5 / ts(ISO8601 UTC)
class ScreenshotHelper : public QObject {
    Q_OBJECT

   public:
    explicit ScreenshotHelper(QObject* parent = nullptr);

    // 截图并落盘 + 记录 manifest
    // item: 待截图的 QQuickItem(通常为 PageHarness 或其子项)
    // tag: 用例标识,作为 PNG 文件名(不含扩展名)与 manifest 的 case/page 字段
    // theme: 主题标识("light"/"dark"),空字符串表示未指定
    // 返回:true 表示截图成功且 PNG + manifest 行已写入;false 表示失败
    Q_INVOKABLE bool capture(QQuickItem* item, const QString& tag, const QString& theme = QString());

    // 截图整个窗口(TestCase 窗口的 contentItem),包含 Overlay 层
    // 用于 Popup/Dialog 类组件视觉用例:Popup 内容渲染在窗口 Overlay 上,
    // grabToItem(harness) 无法捕获 Overlay 内容;而 QQuickOverlay 是 QQuickWindow
    // contentItem 的子项,grabToImage(contentItem) 可一并捕获 Overlay 及其上 Popup。
    // item: 任意属于目标窗口的 QQuickItem(用于定位 window + 解析 TestCase 名)
    // 其余参数与 capture() 一致
    Q_INVOKABLE bool captureWindow(QQuickItem* item, const QString& tag, const QString& theme = QString());

   private:
    // capture / captureWindow 共用的截图落盘 + manifest 写入实现
    // grab_item: 实际调用 grabToImage 的目标项(capture 传 item 本身,
    //            captureWindow 传 item->window()->contentItem())
    // name_item: 用于 resolveTestName 父链遍历的项(始终是调用方传入的 item,
    //            因 contentItem 是窗口根项,其父链不含 TestCase)
    bool captureImpl(QQuickItem* grab_item, QQuickItem* name_item, const QString& tag, const QString& theme);
    // 解析截图产物根目录(环境变量 QML_UI_ARTIFACT_DIR)
    // 返回空字符串表示未配置(非 ctest 运行场景)
    QString resolveArtifactDir() const;

    // 推断当前 TestCase 名(用于产物子目录与 manifest test 字段)
    // 优先从 item 父链查找 name 属性(匹配 "tst_" 前缀,获取 TestCase 名);
    // 回退 QTest::currentTestFunction()(返回函数名);最终回退 "unknown"
    QString resolveTestName(QQuickItem* item) const;
};

}  // namespace tests
}  // namespace gdl
