#include <QtTest>
#include <QFile>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "TestStubs.h"

using namespace gdl::tests;

class TstSingleDelete : public QObject {
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
		fakeBrowser_->setTaskDeletionOutcome(FakeDeletionOutcome::kComplete);
		toastManager_->clearHistory();
	}

	void test_complete_result_shows_success_feedback_data() {
		QTest::addColumn<bool>("deleteContent");
		QTest::addColumn<QString>("expectedMessage");

		QTest::newRow("record-only") << false << QStringLiteral("Task record was removed.");
		QTest::newRow("delete-content") << true
									<< QStringLiteral("Task and downloaded content were removed.");
	}

	void test_complete_result_shows_success_feedback() {
		QFETCH(bool, deleteContent);
		QFETCH(QString, expectedMessage);

		QQuickWindow window;
		window.resize(900, 260);
		window.show();

		QScopedPointer<QObject> page(createPage(window, 0));
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* dialog = page->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "单任务删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog, deleteContent);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveTask"));
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(0).toInt(), 0);
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(1).toString(), QStringLiteral("single-gid"));
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(2).toBool(), deleteContent);
		QCOMPARE(toastManager_->lastType(), QStringLiteral("success"));
		QCOMPARE(toastManager_->lastMessage(), expectedMessage);
	}

	void test_partial_result_shows_warning_feedback() {
		fakeBrowser_->setTaskDeletionOutcome(FakeDeletionOutcome::kPartial);
		QQuickWindow window;
		window.resize(900, 260);
		window.show();

		QScopedPointer<QObject> page(createPage(window, 2));
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* dialog = page->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "单任务删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog, true);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveStopTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("warning"));
		QCOMPARE(toastManager_->lastMessage(),
				 QStringLiteral("The task was removed, but some downloaded content could not be deleted."));
	}

	void test_failed_result_shows_error_feedback() {
		fakeBrowser_->setTaskDeletionOutcome(FakeDeletionOutcome::kFailed);
		QQuickWindow window;
		window.resize(900, 260);
		window.show();

		QScopedPointer<QObject> page(createPage(window, 1));
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* dialog = page->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "单任务删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog, false);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("Failed to remove the task."));
	}

	void test_invalid_result_shows_safe_error_feedback_data() {
		QTest::addColumn<QVariant>("result");

		QTest::newRow("invalid") << QVariant{};
		QTest::newRow("null") << QVariant::fromValue<QObject*>(nullptr);
	}

	void test_invalid_result_shows_safe_error_feedback() {
		QFETCH(QVariant, result);
		fakeBrowser_->setTaskDeletionResult(result);
		QQuickWindow window;
		window.resize(900, 260);
		window.show();

		QScopedPointer<QObject> page(createPage(window, 0));
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* dialog = page->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "单任务删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog, false);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("The task could not be removed."));
		QCOMPARE(toastManager_->messageCount(), 1);
	}

	void test_contradictory_result_shows_safe_error_feedback() {
		fakeBrowser_->setTaskDeletionResult(
			QVariantMap{{QStringLiteral("completeSuccess"), true},
						{QStringLiteral("partialSuccess"), true}});
		QQuickWindow window;
		window.resize(900, 260);
		window.show();

		QScopedPointer<QObject> page(createPage(window, 0));
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* dialog = page->findChild<QObject*>("deleteConfirmDialog");
		QVERIFY2(dialog, "单任务删除应复用 DeleteConfirmDialog");
		invokeDeleteButton(dialog, false);

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RemoveTask"));
		QCOMPARE(toastManager_->lastType(), QStringLiteral("error"));
		QCOMPARE(toastManager_->lastMessage(), QStringLiteral("The task could not be removed."));
		QCOMPARE(toastManager_->messageCount(), 1);
	}

	void test_stopped_deletion_uses_structured_result_as_the_only_toast_source() {
		QFile source(QStringLiteral(SOURCE_ROOT "/src/App/ui/Browser/browser_manager.cxx"));
		QVERIFY2(source.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(source.errorString()));
		const QString text = QString::fromUtf8(source.readAll());
		const qsizetype start = text.indexOf(QStringLiteral("TaskDeletionResult BrowserManagerImpl::RemoveStopTaskResult"));
		const qsizetype end = text.indexOf(QStringLiteral("void BrowserManagerImpl::RefreshTaskList"), start);
		QVERIFY(start >= 0);
		QVERIFY(end > start);

		const QString stoppedDeletionSection = text.mid(start, end - start);
		QVERIFY2(!stoppedDeletionSection.contains(QStringLiteral("Q_EMIT sigErrorMessage")),
				 "Stopped deletion must return structured status without also emitting a legacy error toast");
	}

   private:
	QObject* createPage(QQuickWindow& window, int pageType) {
		QQmlComponent component(&engine_, QUrl("qrc:/qml/CommonComponents/GDownloadViewPage.qml"));
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
		item->setSize(QSizeF(window.width(), window.height()));
		item->setProperty("pageType", pageType);
		return object;
	}

	void invokeDeleteButton(QObject* dialog, bool deleteContent) {
		dialog->setProperty("currentTaskId", QStringLiteral("single-gid"));
		dialog->setProperty("deleteFileChecked", deleteContent);
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

QTEST_MAIN(TstSingleDelete)
#include "tst_single_delete.moc"
