#pragma once

#include <QQmlEngine>
#include <QString>
#include <QQuickItem>

namespace gdl {
namespace tests {

class FakeBrowserManager;
class FakeSettingsManager;

// 集成测试共享引擎设置:注册全部 QML 单例 + 枚举 + 字体,使真实 QML 页面可加载
//
// 镜像 tests/qml_ui/visual/main.cpp 的 TestSetup::qmlEngineAvailable,关键差异:
//   - BrowserManager 单例 → FakeBrowserManager(记录 RPC 调用历史)
//   - SettingsManager 单例 → FakeSettingsManager(记录 setter 写入历史)
//   - 其余单例(GTheme / ToastManager / NetWorkDiskManager / UpdateManager /
//     UtilsToolsManager / LanguageManager / FolderHistoryModel)复用 TestStubs.h 桩
//   - 枚举 GThemeType / SegoeFluentIcons 显式注册(静态库 moc 初始化器被链接器丢弃)
//   - FluentIcons 字体加载到 QFontDatabase + 设上下文属性
//
// 调用方拥有 engine / fakeBrowser / fakeSettings 的生命周期(通常作为测试类成员)。
// 本函数仅在 engine 上注册类型与上下文属性,不接管所有权。
void setupIntegrationEngine(QQmlEngine* engine, FakeBrowserManager* fakeBrowser,
							FakeSettingsManager* fakeSettings);

// 加载 QML 组件并按 objectName 查找子控件。失败返回 nullptr 并输出错误到 qWarning
template <typename T = QQuickItem>
T* loadAndFind(QQmlEngine* engine, const QString& qmlUrl, const QString& objectName) {
	QQmlComponent component(engine, QUrl(qmlUrl));
	if (component.isError()) {
		qWarning("loadAndFind: 组件加载失败 %s: %s", qPrintable(qmlUrl),
				 qPrintable(component.errorString()));
		return nullptr;
	}
	QObject* obj = component.create();
	if (!obj) {
		qWarning("loadAndFind: 组件实例化失败 %s", qPrintable(qmlUrl));
		return nullptr;
	}
	// 组件实例由调用方管理;此处用 findChild 定位 objectName
	auto* target = obj->findChild<T*>(objectName);
	if (!target) {
		qWarning("loadAndFind: 未找到 objectName=%s (QML=%s)", qPrintable(objectName),
				 qPrintable(qmlUrl));
	}
	return target;
}

}  // namespace tests
}  // namespace gdl
