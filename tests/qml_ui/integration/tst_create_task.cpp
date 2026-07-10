#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "TestStubs.h"

using namespace gdl::tests;

// brower_view 桩:TaskDialogPage 提交后调 brower_view.index = 0 / switchDownloadPage(0)
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

// ClipboardWatcher 桩:TaskDialogPage Component.onCompleted 调 GetClipboardText,
// Connections 监听 clipboardChanged(桩不发射,不污染 input.text)
class StubClipboardWatcher : public QObject {
	Q_OBJECT
   public:
	explicit StubClipboardWatcher(QObject* p = nullptr) : QObject(p) {}
	Q_INVOKABLE QString GetClipboardText() const { return QString(); }
   Q_SIGNALS:
	void clipboardChanged();
};

// tst_create_task:验证新建任务对话框提交 → BrowserManager.AddHttpTask 调用
//
// 加载 TaskDialogPage.qml(Popup),向 inputUrl(TextArea)写入 URL,点 btnCreateTask,
// 断言 FakeBrowserManager.lastRpcMethod()=="AddHttpTask" 且参数含输入的 URL。
//
// TaskDialogPage 引用 brower_view / ClipboardWatcher / mainWindow(生产环境为 QML id 或
// context property),此处以 context property 注入桩,使页面可加载、提交可执行。
class TstCreateTask : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		toastManager_ = setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
		// 额外上下文属性:TaskDialogPage 引用 brower_view / ClipboardWatcher / mainWindow
		stubBrowserView_ = new StubBrowserView(this);
		stubClipboard_ = new StubClipboardWatcher(this);
		stubMainWindow_ = new QObject(this);	// 占位父对象
		engine_.rootContext()->setContextProperty("brower_view", stubBrowserView_);
		engine_.rootContext()->setContextProperty("ClipboardWatcher", stubClipboard_);
		engine_.rootContext()->setContextProperty("mainWindow", stubMainWindow_);
	}

	void init() {
		fakeBrowser_->clearHistory();
		fakeBrowser_->setAddTaskResult(true);
		fakeSettings_->clearHistory();
		toastManager_->clearHistory();
		stubBrowserView_->setIndex(-1);
	}

	void test_addUri_called() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/TaskDialogPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "TaskDialogPage 实例化失败");

		// 找到 URL TextArea(inputUrl),写入测试 URL
		auto* inputUrl = dialog->findChild<QQuickItem*>("inputUrl");
		QVERIFY2(inputUrl, "未找到 inputUrl");
		QVERIFY2(inputUrl->setProperty("text", "http://example.com/file.zip"),
				 "设置 inputUrl.text 失败");

		// 找到 btnCreateTask,点击 → geturls()/getOptions() → BrowserManager.AddHttpTask
		auto* btnCreateTask = dialog->findChild<QQuickItem*>("btnCreateTask");
		QVERIFY2(btnCreateTask, "未找到 btnCreateTask");
		QMetaObject::invokeMethod(btnCreateTask, "clicked");

		// 断言 AddHttpTask 被调用,且 URL 在参数中
		QTRY_COMPARE_WITH_TIMEOUT(fakeBrowser_->lastRpcMethod(),
								  QStringLiteral("AddHttpTask"), 1000);
		const auto args = fakeBrowser_->lastRpcCall().args;
		QVERIFY2(args.size() == 2, "AddHttpTask 应有 2 个参数(urls, options)");
		// args[0] = QVariantList(urls),应含测试 URL
		const auto urls = args.at(0).toList();
		QVERIFY2(!urls.isEmpty(), "URL 列表不应为空");
		bool url_found = false;
		for (const auto& u : urls) {
			if (u.toString().contains(QStringLiteral("example.com/file.zip"))) {
				url_found = true;
				break;
			}
		}
		QVERIFY2(url_found, "AddHttpTask 参数应含输入的 URL");

		const auto options = args.at(1).toMap();
		QCOMPARE(options.value(QStringLiteral("split")).toString(), QStringLiteral("64"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("success"));
		QVERIFY(!toastManager_->lastMessage().isEmpty());
	}

	void test_blank_lines_are_trimmed_and_filtered() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/TaskDialogPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "TaskDialogPage 实例化失败");

		auto* inputUrl = dialog->findChild<QQuickItem*>("inputUrl");
		QVERIFY(inputUrl);
		inputUrl->setProperty("text",
						  "  https://example.com/one.zip  \n\n\t\r\n https://example.com/two.zip ");
		auto* btnCreateTask = dialog->findChild<QQuickItem*>("btnCreateTask");
		QVERIFY(btnCreateTask);
		QMetaObject::invokeMethod(btnCreateTask, "clicked");

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("AddHttpTask"));
		const auto urls = fakeBrowser_->lastRpcCall().args.at(0).toList();
		QCOMPARE(urls.size(), 2);
		QCOMPARE(urls.at(0).toString(), QStringLiteral("https://example.com/one.zip"));
		QCOMPARE(urls.at(1).toString(), QStringLiteral("https://example.com/two.zip"));
	}

	void test_empty_url_input_shows_local_error() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/TaskDialogPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "TaskDialogPage 实例化失败");

		auto* inputUrl = dialog->findChild<QQuickItem*>("inputUrl");
		QVERIFY(inputUrl);
		inputUrl->setProperty("text", " \n\t\r\n ");
		auto* btnCreateTask = dialog->findChild<QQuickItem*>("btnCreateTask");
		QVERIFY(btnCreateTask);
		QMetaObject::invokeMethod(btnCreateTask, "clicked");

		QTest::qWait(50);
		QCOMPARE(fakeBrowser_->rpcCallCount(), 0);
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QVERIFY(!toastManager_->lastMessage().isEmpty());
	}

	void test_missing_torrent_file_shows_local_error() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/TaskDialogPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "TaskDialogPage 实例化失败");
		dialog->setProperty("initialTab", 1);

		auto* btnCreateTask = dialog->findChild<QQuickItem*>("btnCreateTask");
		QVERIFY(btnCreateTask);
		QMetaObject::invokeMethod(btnCreateTask, "clicked");

		QTest::qWait(50);
		QCOMPARE(fakeBrowser_->rpcCallCount(), 0);
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QVERIFY(!toastManager_->lastMessage().isEmpty());
	}

	void test_addUri_failure_stays_on_dialog() {
		fakeBrowser_->setAddTaskResult(false);

		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/TaskDialogPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "TaskDialogPage 实例化失败");

		auto* inputUrl = dialog->findChild<QQuickItem*>("inputUrl");
		QVERIFY2(inputUrl, "未找到 inputUrl");
		QVERIFY2(inputUrl->setProperty("text", "http://example.com/fail.zip"),
				 "设置 inputUrl.text 失败");

		auto* btnCreateTask = dialog->findChild<QQuickItem*>("btnCreateTask");
		QVERIFY2(btnCreateTask, "未找到 btnCreateTask");
		QMetaObject::invokeMethod(btnCreateTask, "clicked");

		QTRY_COMPARE_WITH_TIMEOUT(fakeBrowser_->lastRpcMethod(),
								  QStringLiteral("AddHttpTask"), 1000);
		QCOMPARE(stubBrowserView_->index(), -1);
	}

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
	StubBrowserView* stubBrowserView_ = nullptr;
	StubClipboardWatcher* stubClipboard_ = nullptr;
	QObject* stubMainWindow_ = nullptr;
	TestToastManager* toastManager_ = nullptr;
};

QTEST_MAIN(TstCreateTask)
#include "tst_create_task.moc"
