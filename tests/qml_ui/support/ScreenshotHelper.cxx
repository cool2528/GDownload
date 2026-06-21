#include "ScreenshotHelper.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTest>
#include <QTimer>

namespace gdl {
namespace tests {

ScreenshotHelper::ScreenshotHelper(QObject* parent) : QObject(parent) {}

QString ScreenshotHelper::resolveArtifactDir() const {
    // 由 CMake ENVIRONMENT 注入(ctest 运行时设置)
    // 未设置时返回空,触发 capture 失败(非 ctest 运行场景不应调用截图)
    return qEnvironmentVariable("QML_UI_ARTIFACT_DIR", QString());
}

QString ScreenshotHelper::resolveTestName(QQuickItem* item) const {
    // 优先从 item 父链查找 TestCase 的 name 属性
    // TestCase 在 QML 中声明 name: "tst_xxx",C++ 侧通过元对象读取
    // QTest::currentTestFunction() 仅返回函数名(如 test_basic_light),
    // 非 TestCase 名(如 tst_settings_pages),故优先走父链
    for (QQuickItem* p = item; p != nullptr; p = p->parentItem()) {
        const QVariant nameVar = p->property("name");
        if (!nameVar.isValid() || !nameVar.canConvert<QString>()) {
            continue;
        }
        const QString name = nameVar.toString();
        if (name.startsWith(QStringLiteral("tst_"))) {
            return name;
        }
    }
    // 回退:QuickTest 上下文中返回当前测试函数名
    const char* fn = QTest::currentTestFunction();
    if (fn != nullptr && fn[0] != '\0') {
        return QString::fromUtf8(fn);
    }
    // 最终回退
    return QStringLiteral("unknown");
}

bool ScreenshotHelper::capture(QQuickItem* item, const QString& tag, const QString& theme) {
    if (item == nullptr) {
        qWarning() << "ScreenshotHelper::capture: null item";
        return false;
    }
    return captureImpl(item, item, tag, theme);
}

bool ScreenshotHelper::captureWindow(QQuickItem* item, const QString& tag, const QString& theme) {
    if (item == nullptr) {
        qWarning() << "ScreenshotHelper::captureWindow: null item";
        return false;
    }
    QQuickWindow* window = item->window();
    if (window == nullptr) {
        qWarning() << "ScreenshotHelper::captureWindow: item has no associated window";
        return false;
    }
    QQuickItem* contentItem = window->contentItem();
    if (contentItem == nullptr) {
        qWarning() << "ScreenshotHelper::captureWindow: window has no contentItem";
        return false;
    }
    // grab 目标切到窗口 contentItem(含 Overlay 层,Popup/Dialog 渲染其上);
    // testName 解析仍走原 item 父链(contentItem 是窗口根,父链不含 TestCase)
    return captureImpl(contentItem, item, tag, theme);
}

bool ScreenshotHelper::captureImpl(QQuickItem* grab_item, QQuickItem* name_item, const QString& tag,
                                   const QString& theme) {
    if (grab_item == nullptr) {
        qWarning() << "ScreenshotHelper::captureImpl: null grab_item";
        return false;
    }

    const QString artifactDir = resolveArtifactDir();
    if (artifactDir.isEmpty()) {
        // 产物目录未配置(非 ctest 运行场景),无法写盘
        qWarning() << "ScreenshotHelper::captureImpl: artifact dir not configured (QML_UI_ARTIFACT_DIR unset)";
        return false;
    }

    // 路径净化:tag 用于拼文件名,必须屏蔽路径分隔符与 ".." 等目录穿越片段
    // 仅保留字母数字下划线短横,其余(含 / \ .. 空格)统一替换为下划线
    QString sanitizedTag = tag;
    sanitizedTag.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));

    const QString testName = resolveTestName(name_item);
    const QString caseDir = artifactDir + QLatin1Char('/') + testName;
    // 创建测试子目录(已存在则为 no-op)
    if (!QDir().mkpath(caseDir)) {
        qWarning() << "ScreenshotHelper::captureImpl: failed to create dir" << caseDir;
        return false;
    }

    // 异步截图:grabToImage 返回共享指针,ready 信号触发时图像可用
    QSharedPointer<QQuickItemGrabResult> grabResult = grab_item->grabToImage();
    if (grabResult == nullptr) {
        qWarning() << "ScreenshotHelper::captureImpl: grabToImage returned null";
        return false;
    }

    // QEventLoop 阻塞等待 ready 信号(同步语义,便于 QML 测试 verify 返回值)
    QEventLoop loop;
    bool succeeded = false;
    QObject::connect(grabResult.data(), &QQuickItemGrabResult::ready, &loop,
                     [&]() {
                         succeeded = true;
                         loop.quit();
                     });
    // 5s 超时保护:offscreen 平台卡死时不永久阻塞测试
    QTimer::singleShot(5000, &loop, [&]() { loop.quit(); });
    loop.exec();

    if (!succeeded) {
        // ready 未在超时内触发(grab 失败或 offscreen 平台未渲染)
        qWarning() << "ScreenshotHelper::captureImpl: grab timed out after 5s for tag" << sanitizedTag;
        return false;
    }

    const QImage img = grabResult->image();
    if (img.isNull()) {
        qWarning() << "ScreenshotHelper::captureImpl: image is null after grab";
        return false;
    }

    // PNG 文件名:净化后的 tag + .png
    const QString pngFileName = sanitizedTag + QStringLiteral(".png");
    const QString fullPath = caseDir + QLatin1Char('/') + pngFileName;
    if (!img.save(fullPath, "PNG")) {
        qWarning() << "ScreenshotHelper::captureImpl: failed to save PNG to" << fullPath;
        return false;
    }

    // 计算 MD5(读回 PNG 文件字节,保证与落盘内容一致)
    QFile pngFile(fullPath);
    if (!pngFile.open(QIODevice::ReadOnly)) {
        qWarning() << "ScreenshotHelper::captureImpl: failed to open PNG for MD5 read" << fullPath;
        return false;
    }
    const QByteArray pngBytes = pngFile.readAll();
    pngFile.close();
    const QByteArray md5hex =
        QCryptographicHash::hash(pngBytes, QCryptographicHash::Md5).toHex();

    // manifest 相对路径:testName/sanitizedTag.png(相对 artifactDir,正斜杠保证跨平台一致)
    const QString relPath = testName + QLatin1Char('/') + pngFileName;

    // ISO8601 UTC 时间戳
    const QString ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // 构建 JSONL 行(case/page 均填净化后 tag,与文件名保持一致;Phase 3 视觉用例若需区分 page 可扩展 API)
    QJsonObject line;
    line[QStringLiteral("test")] = testName;
    line[QStringLiteral("case")] = sanitizedTag;
    line[QStringLiteral("page")] = sanitizedTag;
    line[QStringLiteral("theme")] = theme;
    line[QStringLiteral("file")] = relPath;
    line[QStringLiteral("md5")] = QString::fromUtf8(md5hex);
    line[QStringLiteral("ts")] = ts;
    const QByteArray jsonLine =
        QJsonDocument(line).toJson(QJsonDocument::Compact);

    // 追加写入 manifest.jsonl(每行一个紧凑 JSON 对象 + 换行)
    // 不使用 QIODevice::Text:Windows 下 Text 模式会把 "\n" 转 "\r\n",破坏跨平台一致性
    const QString manifestPath =
        artifactDir + QLatin1Char('/') + QStringLiteral("manifest.jsonl");
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::Append)) {
        qWarning() << "ScreenshotHelper::captureImpl: failed to open manifest.jsonl for append";
        return false;
    }
    manifestFile.write(jsonLine);
    manifestFile.write("\n");
    manifestFile.close();

    return true;
}

}  // namespace tests
}  // namespace gdl
