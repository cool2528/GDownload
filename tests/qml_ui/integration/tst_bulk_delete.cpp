#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>

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
		fakeBrowser_->setRemoveAllTaskResult(true);
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
	}

	void test_failure_shows_error_feedback() {
		fakeBrowser_->setRemoveAllTaskResult(false);
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
		QVERIFY(!toastManager_->lastMessage().isEmpty());
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
