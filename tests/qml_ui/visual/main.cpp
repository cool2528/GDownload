#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QFontDatabase>

#include "ScreenshotHelper.h"
#include "TestStubs.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"

// QuickTest setup:QML 引擎就绪时注入视觉测试所需全部 QML 单例与上下文属性
//
// Phase 3(Task 6)视觉用例加载真实 QML 页面(DownloadPageView / NavigatorView /
// mainWindow / NetDiskPageView),这些页面通过 `import gdl.sdk 1.0` 引用
// GTheme / BrowserManager / SettingsManager / ToastManager 等单例。
//
// 当前架构:真实单例 C++ 实现(BrowserManagerImpl / SettingsImpl / GTheme)符号
// 仅存在于 gdownload.exe,qml_ui_visual 无法链接。Task 9 将重构主项目为库以
// 提供 Impl 符号;在那之前,这里用 TestStubs.h 中的桩对象顶替:
//   - TestGTheme           -> "GTheme"           (颜色/尺寸/字号令牌 + theme 切换)
//   - TestBrowserManager    -> "BrowserManager"   (GetActiveDownloadModel 等返回 nullptr)
//   - TestSettingsManager   -> "SettingsManager"  (qRememberWindowPosition 等 qXxx 默认值)
//   - TestToastManager      -> "ToastManager"     (ShowError 等空实现)
//   - TestNetWorkDiskManager-> "NetWorkDiskManager"(ParseShareUrl 空实现)
//   - TestUpdateManager     -> "UpdateManager"    (空对象)
//   - TestUtilsToolsManager -> "UtilsToolsManager"(HideMacOsxWindowStandardButtons 等)
//   - TestLanguageManager   -> "LanguageManager"  (空对象)
//
// 枚举 GThemeType / SegoeFluentIcons 通过 Q_NAMESPACE + Q_ENUM_NS + QML_NAMED_ELEMENT
// 声明。AUTOMOC 理论上会生成静态初始化器自动注册到 "gdl.sdk",但 qml_ui_support 是
// 静态库,MSVC 链接器会丢弃未被显式引用的 moc object,导致初始化器不执行。这里复刻
// mainwindow.cxx InitQmlEngine 末尾的 qmlRegisterUncreatableMetaObject 显式注册,
// 确保 QML 端 GThemeType.ThemeMode.Dark / SegoeFluentIcons.<Icon> 可解析。
//
// 桩对象生命周期:绑定到 engine,引擎销毁时随 engine 一起释放。
class TestSetup : public QObject {
	Q_OBJECT

   public slots:
	void qmlEngineAvailable(QQmlEngine* engine) {
		// ScreenshotHelper:QML 端通过 Screenshot.capture(item, "tag", "theme") 触发截图
		auto* helper = new gdl::tests::ScreenshotHelper(engine);
		engine->rootContext()->setContextProperty("Screenshot", helper);

		// 桩单例:用 qmlRegisterSingletonInstance 注册到 "gdl.sdk" 模块
		// 模块名与生产环境一致,QML 端 `import gdl.sdk 1.0` 即可解析
		// 桩对象 parent 设为 engine,引擎销毁时自动释放
		auto* g_theme = new gdl::tests::TestGTheme(engine);
		auto* browser_mgr = new gdl::tests::TestBrowserManager(engine);
		auto* settings_mgr = new gdl::tests::TestSettingsManager(engine);
		auto* toast_mgr = new gdl::tests::TestToastManager(engine);
		auto* netdisk_mgr = new gdl::tests::TestNetWorkDiskManager(engine);
		auto* update_mgr = new gdl::tests::TestUpdateManager(engine);
		auto* utils_mgr = new gdl::tests::TestUtilsToolsManager(engine);
		auto* lang_mgr = new gdl::tests::TestLanguageManager(engine);

		qmlRegisterSingletonInstance<gdl::tests::TestGTheme>("gdl.sdk", 1, 0, "GTheme", g_theme);
		qmlRegisterSingletonInstance<gdl::tests::TestBrowserManager>("gdl.sdk", 1, 0, "BrowserManager",
																	 browser_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestSettingsManager>("gdl.sdk", 1, 0, "SettingsManager",
																	  settings_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestToastManager>("gdl.sdk", 1, 0, "ToastManager", toast_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestNetWorkDiskManager>("gdl.sdk", 1, 0, "NetWorkDiskManager",
																		 netdisk_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestUpdateManager>("gdl.sdk", 1, 0, "UpdateManager",
																	update_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestUtilsToolsManager>("gdl.sdk", 1, 0, "UtilsToolsManager",
																		utils_mgr);
		qmlRegisterSingletonInstance<gdl::tests::TestLanguageManager>("gdl.sdk", 1, 0, "LanguageManager",
																	  lang_mgr);

		// 枚举注册:复刻 mainwindow.cxx InitQmlEngine 末尾的 qmlRegisterUncreatableMetaObject
		// 显式注册。GThemeType / SegoeFluentIcons 虽经 QML_NAMED_ELEMENT 声明,但静态库
		// 链接器丢弃未引用 moc object,静态初始化器不执行,QML 端报
		// ReferenceError: GThemeType is not defined。显式注册确保 PageHarness 切主题
		// (GThemeType.ThemeMode.kLight/kDark)与 NavigatorView 图标(SegoeFluentIcons.X)可用。
		qmlRegisterUncreatableMetaObject(GThemeType::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
										 "GThemeType", "GThemeType enum");
		qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, GEXPORT_MODULE_URL, 1, 0,
										 "SegoeFluentIcons", "SegoeFluentIcons enum");

		// FolderHistoryModel:NavigatorView 内 Component 链
		// (TaskDialogPage -> TaskGeneralOptionsCard -> FolderSelector) 引用该类型,
		// 缺注册会导致 FolderSelector.qml 解析失败、整链 Component 不可用,
		// NavigatorView 加载中断、截图空白。复刻 mainwindow.cxx 的 qmlRegisterType
		// 注册桩 TestFolderHistoryModel(空列表模型,仅暴露 maxHistoryCount 属性)。
		qmlRegisterType<gdl::tests::TestFolderHistoryModel>("gdl.sdk", 1, 0, "FolderHistoryModel");

		// mainWindow.qml 顶层是 FramelessWindow,依赖 FramelessHelper QML 模块。
		// FramelessHelper 需在 C++ 端调用 FramelessHelper::Quick::registerTypes(engine)
		// 注册,且其 Core 层会在 Win32 平台安装窗口 proc 钩子。offscreen 平台无原生
		// 窗口,加载 mainWindow.qml 会触发 FramelessHelper ASSERT(qtWindowProc) 崩溃。
		// 故视觉用例不注册 FramelessHelper,mainWindow.qml Loader 加载失败,
		// 截图为空白 harness(3674 字节)。这符合 tst_main_views.qml 注释中的设计:
		// "mainWindow 依赖 FramelessHelper QML 模块;若测试环境未注册,Loader 失败,
		//  截图为空白 harness,不影响其他用例产出"。Task 9 重构主项目为库后,可在真实
		// 窗口环境中补充 mainWindow 视觉用例。

		// FluentIcons 字体加载 + 上下文属性:
		// 生产代码 mainwindow.cxx InitFont() 从 "://font/SegoeFluentIcons.ttf" 加载
		// 字体到 QFontDatabase,取字体族名设为 FluentIcons 上下文属性。
		// 视觉用例 qml_ui_visual 已通过 resource_icons.qrc 打包同字体,这里复刻:
		//   - addApplicationFont 注册字体到 QFontDatabase
		//   - 设置 FluentIcons 上下文属性为字体族名
		// FontIcon.qml 直接用 font.family: "Segoe Fluent Icons" 引用,故即使不设
		// 上下文属性也能渲染;但保持与生产一致以防其他组件通过 FluentIcons 引用。
		const int font_id = QFontDatabase::addApplicationFont(QStringLiteral("://font/SegoeFluentIcons.ttf"));
		if (font_id != -1) {
			const auto families = QFontDatabase::applicationFontFamilies(font_id);
			if (!families.isEmpty()) {
				engine->rootContext()->setContextProperty("FluentIcons", families.first());
			}
		} else {
			// 字体加载失败回退:Windows 11 系统自带 "Segoe Fluent Icons",族名直接设
			engine->rootContext()->setContextProperty("FluentIcons", QStringLiteral("Segoe Fluent Icons"));
		}
	}
};

QUICK_TEST_MAIN_WITH_SETUP(qml_ui_visual, TestSetup)

#include "main.moc"
