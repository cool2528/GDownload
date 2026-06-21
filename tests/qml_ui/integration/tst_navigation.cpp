#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"

using namespace gdl::tests;

// brower_view 桩:NavigatorView 点击导航按钮时写 brower_view.index
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
	Q_INVOKABLE void switchDownloadPage(int i) { setIndex(i); }
	Q_INVOKABLE void switchSettingPage(int i) { Q_UNUSED(i); }
   Q_SIGNALS:
	void indexChanged();

   private:
	int index_ = -1;
};

// tst_navigation:验证 NavigatorView 导航按钮点击切换主视图索引
//
// NavigatorView 的 navDownloading 按钮 onClicked 执行 `brower_view.index = 0`(切到下载页),
// setting 按钮(无 objectName)切 index=1,navHome 打开外链(不切视图)。
// mainWindow.qml 顶层是 FramelessWindow,依赖 FramelessHelper QML 模块,offscreen 平台
// 加载会崩溃,故本用例直接加载 NavigatorView + 以 context property 注入 brower_view 桩,
// 点击 navDownloading 断言 brower_view.index 变为 0。
//
// 局限:setting / help / addTask 按钮无 objectName(Task 9 仅注入 navHome/navDownloading),
// 无法 findChild 定位,故仅验证 navDownloading→index=0 路由。navHome 打开外链
// (Qt.openUrlExternally)在测试环境不便断言,跳过。
class TstNavigation : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
		stubBrowserView_ = new StubBrowserView(this);
		stubMainWindow_ = new QObject(this);
		engine_.rootContext()->setContextProperty("brower_view", stubBrowserView_);
		engine_.rootContext()->setContextProperty("mainWindow", stubMainWindow_);
	}

	void init() {
		stubBrowserView_->setIndex(-1);
		fakeBrowser_->clearHistory();
	}

	void test_main_views_routing() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/Navigator/NavigatorView.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> navigator(comp.create());
		QVERIFY2(!navigator.isNull(), "NavigatorView 实例化失败");

		// 初始 index 应为 -1(桩默认)
		QCOMPARE(stubBrowserView_->index(), -1);

		// 找到 navDownloading 按钮,点击 → brower_view.index = 0
		auto* navDownloading = navigator->findChild<QQuickItem*>("navDownloading");
		QVERIFY2(navDownloading, "未找到 navDownloading");
		QMetaObject::invokeMethod(navDownloading, "clicked");

		// 断言 index 切换为 0(下载页)
		QTRY_COMPARE_WITH_TIMEOUT(stubBrowserView_->index(), 0, 1000);
	}

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
	StubBrowserView* stubBrowserView_ = nullptr;
	QObject* stubMainWindow_ = nullptr;
};

QTEST_MAIN(TstNavigation)
#include "tst_navigation.moc"
