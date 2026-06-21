#pragma once

#include <QObject>
#include <QQuickItem>
#include <QString>

namespace gdl {
namespace tests {

// 测试期截图辅助
// QML 端通过 Screenshot.capture(item, "tag") 调用,Phase 3 填充:
//   1. item->grabToImage() 异步截图
//   2. PNG 写入 $QML_UI_ARTIFACT_DIR/<test>/<tag>.png
//   3. 同步 append 一行 JSON 到 manifest.jsonl
// Phase 1:仅占位,capture() 为空实现,smoke 测试不依赖截图产物
class ScreenshotHelper : public QObject {
    Q_OBJECT

   public:
    explicit ScreenshotHelper(QObject* parent = nullptr) : QObject(parent) {}

    // 截图并落盘 + 记录 manifest
    // Phase 1 stub:空实现,Phase 3 填实
    Q_INVOKABLE void capture(QQuickItem* item, const QString& tag) {
        Q_UNUSED(item);
        Q_UNUSED(tag);
    }
};

}  // namespace tests
}  // namespace gdl
