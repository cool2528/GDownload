#include <QtTest>
#include <QColor>
#include <QQmlEngine>
#include <QtQml/qqml.h>

#include "TestStubs.h"
#include "Definitions/appDef.h"
#include "Definitions/fluentEnumDef.h"

using namespace gdl::tests;

// tst_theme_toggle:验证 GTheme 主题切换后核心 token 颜色变化
//
// GTheme 真实实现(theme.cpp)符号仅存于 gdownload.exe(不可链接),故用
// TestStubs::TestGTheme 复刻的颜色逻辑(深浅色取值与 theme.cpp 一致)验证切换机制:
//   Settheme(kDark=2) → dark_=true → darkChanged → bgBase/textPrimary/borderInfo 切换
//
// 测试方式:C++ 直接读写 TestGTheme 的 theme 属性(Q_PROPERTY int theme WRITE Settheme),
// 抽样断言 bgBase / textPrimary / borderInfo 在深浅色下取值不同,并验证可逆。
// 这验证了主题切换的 token 变化机制(真实 GTheme 的等价逻辑由 TestGTheme 复刻)。
class TstThemeToggle : public QObject {
	Q_OBJECT

   private slots:
	void test_token_changes() {
		qputenv("GDOWNLOAD_TEST", "1");
		QQmlEngine engine;
		TestGTheme gtheme;
		// 注册为 GTheme 单例(与生产一致),枚举显式注册(静态库 moc 初始化器被丢弃)
		qmlRegisterSingletonInstance<TestGTheme>("gdl.sdk", 1, 0, "GTheme", &gtheme);
		qmlRegisterUncreatableMetaObject(GThemeType::staticMetaObject, "gdl.sdk", 1, 0,
										 "GThemeType", "GThemeType enum");
		qmlRegisterUncreatableMetaObject(SegoeFluentIcons::staticMetaObject, "gdl.sdk", 1, 0,
										 "SegoeFluentIcons", "SegoeFluentIcons enum");

		// 浅色(kLight=1)初始态:TestGTheme 默认 dark_=false
		const QColor bgBaseLight = gtheme.property("bgBase").value<QColor>();
		const QColor textPrimaryLight = gtheme.property("textPrimary").value<QColor>();
		const QColor borderInfoLight = gtheme.property("borderInfo").value<QColor>();
		QVERIFY2(bgBaseLight.isValid(), "bgBase 应为有效颜色");

		// 切到深色(kDark=2):setProperty("theme", 2) → Settheme(2) → dark_=true → darkChanged
		QVERIFY2(gtheme.setProperty("theme", 2), "设置 theme=2(深色)失败");
		const QColor bgBaseDark = gtheme.property("bgBase").value<QColor>();
		const QColor textPrimaryDark = gtheme.property("textPrimary").value<QColor>();
		const QColor borderInfoDark = gtheme.property("borderInfo").value<QColor>();

		// 断言三个核心 token 在深浅色下取值不同
		QVERIFY2(bgBaseLight != bgBaseDark, "bgBase 应在深浅色下不同");
		QVERIFY2(textPrimaryLight != textPrimaryDark, "textPrimary 应在深浅色下不同");
		QVERIFY2(borderInfoLight != borderInfoDark, "borderInfo 应在深浅色下不同");

		// 切回浅色验证可逆
		QVERIFY2(gtheme.setProperty("theme", 1), "设置 theme=1(浅色)失败");
		QCOMPARE(gtheme.property("bgBase").value<QColor>(), bgBaseLight);
	}
};

QTEST_MAIN(TstThemeToggle)
#include "tst_theme_toggle.moc"
