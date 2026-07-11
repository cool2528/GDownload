#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>

#include <limits>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "TestStubs.h"

using namespace gdl::tests;

class TstBulkDelete : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		toastManager_ = setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
	}

	void init() {
		fakeBrowser_->clearHistory();
		fakeBrowser_->setBulkDeletionResult(1, 1, 0, 0);
		toastManager_->clearHistory();
	}

	void test_default_keeps_downloaded_files() {
		QCOMPARE(fakeBrowser_->RemoveAllTask(2).value(QStringLiteral("failed")).toInt(), 0);
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(1).toBool(), false);
		fakeBrowser_->clearHistory();

		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 2));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");
		QCOMPARE(dialog->property("batchMode").toBool(), true);
		QCOMPARE(dialog->property("pageType").toInt(), 2);

		dialog->setProperty("deleteFileChecked", false);
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		const auto args = fakeBrowser_->lastRpcCall().args;
		QCOMPARE(args.size(), 2);
		QCOMPARE(args.at(0).toInt(), 2);
		QCOMPARE(args.at(1).toBool(), false);
		QCOMPARE(toastManager_->lastType(), QStringLiteral("success"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("All task records were removed."));
	}

	void test_explicit_checkbox_deletes_files() {
		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 0));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");

		dialog->setProperty("deleteFileChecked", true);
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(1).toBool(), true);
		QCOMPARE(toastManager_->lastType(), QStringLiteral("success"));
		QCOMPARE(toastManager_->lastMessage(),
				 QStringLiteral("All tasks and downloaded content were removed."));
	}

	void test_partial_result_shows_warning_feedback() {
		fakeBrowser_->setBulkDeletionResult(2, 1, 1, 0);
		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 1));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");
		dialog->setProperty("deleteFileChecked", true);
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("warning"));
		QCOMPARE(toastManager_->lastMessage(),
				 QStringLiteral("Some tasks were removed, but some downloaded content could not be deleted."));
	}

	void test_mixed_failed_result_shows_generic_warning_feedback_data() {
		QTest::addColumn<int>("complete");
		QTest::addColumn<int>("partial");
		QTest::addColumn<int>("failed");

		QTest::newRow("complete-and-failed") << 2 << 0 << 1;
		QTest::newRow("partial-and-failed") << 1 << 1 << 1;
	}

	void test_mixed_failed_result_shows_generic_warning_feedback() {
		QFETCH(int, complete);
		QFETCH(int, partial);
		QFETCH(int, failed);
		fakeBrowser_->setBulkDeletionResult(complete + partial + failed, complete, partial, failed);
		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 0));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");
		dialog->setProperty("deleteFileChecked", true);
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("warning"));
		QCOMPARE(toastManager_->lastMessage(),
				 QStringLiteral("Some tasks were removed, but some tasks could not be removed."));
	}

	void test_failure_shows_error_feedback() {
		fakeBrowser_->setBulkDeletionResult(2, 0, 0, 2);
		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 1));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("Failed to remove all tasks."));
	}

	void test_empty_or_invalid_result_shows_error_feedback_data() {
		QTest::addColumn<QVariantMap>("result");

		QTest::newRow("empty") << QVariantMap{{QStringLiteral("total"), 0},
										   {QStringLiteral("complete"), 0},
										   {QStringLiteral("partial"), 0},
										   {QStringLiteral("failed"), 0}};
		QTest::newRow("invalid") << QVariantMap{};
		QTest::newRow("boolean") << QVariantMap{{QStringLiteral("total"), true},
											 {QStringLiteral("complete"), 1},
											 {QStringLiteral("partial"), 0},
											 {QStringLiteral("failed"), 0}};
		QTest::newRow("string") << QVariantMap{{QStringLiteral("total"), QStringLiteral("1")},
										  {QStringLiteral("complete"), QStringLiteral("1")},
										  {QStringLiteral("partial"), 0},
										  {QStringLiteral("failed"), 0}};
		QTest::newRow("fraction") << QVariantMap{{QStringLiteral("total"), 1.0},
											  {QStringLiteral("complete"), 0.5},
											  {QStringLiteral("partial"), 0.5},
											  {QStringLiteral("failed"), 0.0}};
		QTest::newRow("negative") << QVariantMap{{QStringLiteral("total"), 1},
											  {QStringLiteral("complete"), -1},
											  {QStringLiteral("partial"), 1},
											  {QStringLiteral("failed"), 1}};
		QTest::newRow("nan") << QVariantMap{{QStringLiteral("total"), 1.0},
										 {QStringLiteral("complete"), std::numeric_limits<double>::quiet_NaN()},
										 {QStringLiteral("partial"), 0.0},
										 {QStringLiteral("failed"), 0.0}};
	}

	void test_empty_or_invalid_result_shows_error_feedback() {
		QFETCH(QVariantMap, result);
		fakeBrowser_->setBulkDeletionResultMap(result);
		QQuickWindow window;
		window.resize(900, 160);
		window.show();

		QScopedPointer<QObject> title(createTitle(window, 0));
		QVERIFY2(title, "DownloadPageTitle 实例化失败");
		auto* dialog = title->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "批量删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveAllTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("Failed to remove all tasks."));
		QCOMPARE(toastManager_->messageCount(), 1);
	}

   private:
	QObject* createTitle(QQuickWindow& window, int pageType) {
		QQmlComponent component(&engine_, QUrl("qrc:/qml/Browser/DownloadPageTitle.qml"));
		if (component.isError()) {
			qWarning().noquote() << component.errorString();
			return nullptr;
		}
		QObject* object = component.create();
		auto* item = qobject_cast<QQuickItem*>(object);
		if (!item) {
			delete object;
			return nullptr;
		}
		item->setParentItem(window.contentItem());
		item->setWidth(window.width());
		item->setProperty("type", pageType);
		return object;
	}

	void invokeDeleteButton(QObject* dialog) {
		const QVariant buttonData = QVariantMap{};
		QVERIFY2(QMetaObject::invokeMethod(dialog, "buttonClicked", Q_ARG(int, 1),
										 Q_ARG(QVariant, buttonData)),
				 "触发 DeleteConfirmDialog 删除按钮失败");
	}

	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
	TestToastManager* toastManager_ = nullptr;
};

QTEST_MAIN(TstBulkDelete)
#include "tst_bulk_delete.moc"
