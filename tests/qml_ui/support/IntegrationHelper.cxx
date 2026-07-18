#include "IntegrationHelper.h"

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "TestStubs.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"

#include <QQmlContext>
#include <QtQml/qqml.h>

namespace gdl {
namespace tests {

TestToastManager* setupIntegrationEngine(QQmlEngine* engine, FakeBrowserManager* fakeBrowser,
										 FakeSettingsManager* fakeSettings, TestGTheme** theme,
										 TestNetWorkDiskManager** netDisk) {
	// 桩单例(非记录类)parent 设为 engine,引擎销毁时自动释放
	auto* g_theme = new TestGTheme(engine);
	auto* toast_mgr = new TestToastManager(engine);
	auto* netdisk_mgr = new TestNetWorkDiskManager(engine);
	auto* update_mgr = new TestUpdateManager(engine);
	auto* utils_mgr = new TestUtilsToolsManager(engine);
	auto* lang_mgr = new TestLanguageManager(engine);
	if (theme) *theme = g_theme;
	if (netDisk) *netDisk = netdisk_mgr;

	// 记录版单例:BrowserManager / SettingsManager(调用方拥有,fakeBrowser/fakeSettings
	// 通常为测试类成员,生命周期跨多个用例)。这里不转移所有权,parent=nullptr
	qmlRegisterSingletonInstance<FakeBrowserManager>("gdl.sdk", 1, 0, "BrowserManager",
													  fakeBrowser);
	qmlRegisterSingletonInstance<FakeSettingsManager>("gdl.sdk", 1, 0, "SettingsManager",
													  fakeSettings);
	// 其余单例复用 TestStubs 桩
	qmlRegisterSingletonInstance<TestGTheme>("gdl.sdk", 1, 0, "GTheme", g_theme);
	qmlRegisterSingletonInstance<TestToastManager>("gdl.sdk", 1, 0, "ToastManager", toast_mgr);
	qmlRegisterSingletonInstance<TestNetWorkDiskManager>("gdl.sdk", 1, 0, "NetWorkDiskManager",
														  netdisk_mgr);
	qmlRegisterSingletonInstance<TestUpdateManager>("gdl.sdk", 1, 0, "UpdateManager", update_mgr);
	qmlRegisterSingletonInstance<TestUtilsToolsManager>("gdl.sdk", 1, 0, "UtilsToolsManager",
														utils_mgr);
	qmlRegisterSingletonInstance<TestLanguageManager>("gdl.sdk", 1, 0, "LanguageManager",
													   lang_mgr);

	// 枚举注册:静态库 moc 初始化器被 MSVC 链接器丢弃,显式注册确保
	// GThemeType.ThemeMode.kDark / SegoeFluentIcons.<Icon> 在 QML 端可解析
	qmlRegisterUncreatableMetaObject(GThemeType::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
									 "GThemeType", "GThemeType enum");
	qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
									 "SegoeFluentIcons", "SegoeFluentIcons enum");

	// FolderHistoryModel:NavigatorView / TaskDialogPage 内 Component 链引用
	qmlRegisterType<TestFolderHistoryModel>("gdl.sdk", 1, 0, "FolderHistoryModel");

	return toast_mgr;
}

}  // namespace tests
}  // namespace gdl
