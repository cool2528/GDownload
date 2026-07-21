#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "ed2k_manager.h"

using namespace gdl::tests;

// brower_view 桩:NavigatorView 点击 navEd2k 时写 brower_view.index
class StubBrowserView : public QObject {
	Q_OBJECT
	Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
   public:
	explicit StubBrowserView(QObject* p = nullptr) : QObject(p) {}
	int index() const { return index_; }
	void setIndex(int i) {
		if (i != index_) {
			index_ = i;
			Q_EMIT indexChanged();
		}
	}

   Q_SIGNALS:
	void indexChanged();

   private:
	int index_ = -1;
};

// tst_ed2k_center:eD2k 中心页导航入口与 tab 切换的 QML 集成冒烟测试
//
// 断言点(照 Phase 2b Task 8 计划 + Phase 3b Task 8 扩展):
//   1) NavigatorView 的 navEd2k 按钮 clicked 后 brower_view.index 变为 3
//   2) Ed2kCenterPage.qml 能实例化,且 ed2kTabSearch/ed2kTabServers 两个 tab 按钮存在
//   3) 点击 ed2kTabServers 后 currentTabIndex 变为 1
//   4) ed2kTabShares 存在,点击后 currentTabIndex 变为 2(Phase 3b 分享 tab)
//   5) Ed2kSettingPage.qml 能独立实例化(Phase 3b 设置卡片)
//
// 结构照抄 tst_navigation.cpp(StubBrowserView + setupIntegrationEngine)。
//
// 额外依赖说明:Ed2kCenterPage 内嵌的 Ed2kSearchPage/Ed2kServerPage 在属性绑定
// (property resultModel: Ed2kManager.GetSearchResultModel())与
// Component.onCompleted(Ed2kManager.RefreshServers()/RefreshKadStatus())中
// 直接调用 Ed2kManager 的方法,若 "gdl.sdk" 模块下没有注册该单例,组件创建会
// 立即失败。这里直接调用生产代码的 gdl::ui::ed2k::RegisterTypes() 注册真实
// Ed2kManager::Instance(),不调用 Ed2kManager::Init():RegisterTypes 本身只做
// QML 单例注册,不会触发引擎 PubSub 订阅;而引擎层 Ed2kDownloadManager 的对外
// 方法在未 InitEd2kEngine 时(running_ 为 false)全部提前返回、空转安全,因此
// 页面可以正常实例化,属性均为构造期默认值。
class TstEd2kCenter : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
		gdl::ui::ed2k::RegisterTypes(&engine_);
		stubBrowserView_ = new StubBrowserView(this);
		stubMainWindow_ = new QObject(this);
		engine_.rootContext()->setContextProperty("brower_view", stubBrowserView_);
		engine_.rootContext()->setContextProperty("mainWindow", stubMainWindow_);
	}

	void init() {
		stubBrowserView_->setIndex(-1);
		fakeBrowser_->clearHistory();
	}

	// 测试点 1:navEd2k 点击后 brower_view.index == 3
	void navButtonSwitchesToIndex3() {
		QQmlComponent comp(&engine_, QUrl(QStringLiteral("qrc:/qml/Navigator/NavigatorView.qml")));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> navigator(comp.create());
		QVERIFY2(!navigator.isNull(), "NavigatorView 实例化失败");
		auto* navigatorItem = qobject_cast<QQuickItem*>(navigator.data());
		QVERIFY2(navigatorItem, "NavigatorView root must be a QQuickItem");
		navigatorItem->setWidth(74);
		navigatorItem->setHeight(720);
		QCoreApplication::processEvents();

		QCOMPARE(stubBrowserView_->index(), -1);

		auto* btn = navigator->findChild<QQuickItem*>(QStringLiteral("navEd2k"));
		QVERIFY2(btn, "未找到 navEd2k");
		QMetaObject::invokeMethod(btn, "clicked");

		QTRY_COMPARE_WITH_TIMEOUT(stubBrowserView_->index(), 3, 1000);
	}

	// 测试点 2 + 3:Ed2kCenterPage 可实例化、tab 按钮存在,点击后 currentTabIndex 切换
	void tabsSwitch() {
		QQmlComponent comp(&engine_, QUrl(QStringLiteral("qrc:/qml/Browser/Ed2kCenterPage.qml")));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create(engine_.rootContext()));
		QVERIFY2(!page.isNull(), qPrintable(comp.errorString()));

		auto* searchTab = page->findChild<QQuickItem*>(QStringLiteral("ed2kTabSearch"));
		auto* serversTab = page->findChild<QQuickItem*>(QStringLiteral("ed2kTabServers"));
		QVERIFY2(searchTab, "未找到 ed2kTabSearch");
		QVERIFY2(serversTab, "未找到 ed2kTabServers");

		QCOMPARE(page->property("currentTabIndex").toInt(), 0);

		QMetaObject::invokeMethod(serversTab, "clicked");

		QTRY_COMPARE_WITH_TIMEOUT(page->property("currentTabIndex").toInt(), 1, 1000);
	}

	// 测试点 4:ed2kTabShares 存在,点击后 currentTabIndex 切换到 2(Phase 3b 分享 tab)
	void sharesTabSwitchesIndex2() {
		QQmlComponent comp(&engine_, QUrl(QStringLiteral("qrc:/qml/Browser/Ed2kCenterPage.qml")));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create(engine_.rootContext()));
		QVERIFY2(!page.isNull(), qPrintable(comp.errorString()));

		auto* sharesTab = page->findChild<QQuickItem*>(QStringLiteral("ed2kTabShares"));
		QVERIFY2(sharesTab, "未找到 ed2kTabShares");

		QCOMPARE(page->property("currentTabIndex").toInt(), 0);

		QMetaObject::invokeMethod(sharesTab, "clicked");

		QTRY_COMPARE_WITH_TIMEOUT(page->property("currentTabIndex").toInt(), 2, 1000);
	}

	// 测试点 5:Ed2kSettingPage.qml 能独立实例化(Phase 3b 设置卡片,内嵌于 AdvancedSettingPage)
	// FakeSettingsManager 已镜像全部 qEd2kXxx 属性(见 support/FakeSettingsManager.h),
	// 组件创建期读取这些属性求初值不应报错。
	void settingsPageInstantiates() {
		QQmlComponent comp(&engine_, QUrl(QStringLiteral("qrc:/qml/Browser/Ed2kSettingPage.qml")));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create(engine_.rootContext()));
		QVERIFY2(!page.isNull(), qPrintable(comp.errorString()));
	}

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
	StubBrowserView* stubBrowserView_ = nullptr;
	QObject* stubMainWindow_ = nullptr;
};

QTEST_MAIN(TstEd2kCenter)
#include "tst_ed2k_center.moc"
