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

namespace {
QQuickItem* findVisualItem(QQuickItem* root, const QString& objectName) {
	if (!root) return nullptr;
	if (root->objectName() == objectName) return root;
	for (QQuickItem* child : root->childItems()) {
		if (QQuickItem* match = findVisualItem(child, objectName)) return match;
	}
	return nullptr;
}
}  // namespace

class TstDownloadFailure : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
	}

	void init() {
		fakeBrowser_->clearHistory();
		fakeBrowser_->setRetryTaskResult(true);
	}

	void test_failed_task_shows_error_and_retries() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "failed-1"}, {"taskState", 4}, {"fileName", "broken.iso"},
			 {"savePath", "C:/Downloads/broken.iso"}, {"totalSize", "2.0 GB"},
			 {"currentSize", "128 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 6},
			 {"remainingTime", "Stopped"}, {"connections", 0},
			 {"downloadLink", "https://example.com/broken.iso"}, {"errorCode", "3"},
			 {"errorMessage", "Resource not found"}},
		});

		QQuickWindow window;
		window.resize(960, 260);
		window.show();

		QQmlComponent component(&engine_, QUrl("qrc:/qml/CommonComponents/GDownloadViewPage.qml"));
		QVERIFY2(!component.isError(), qPrintable(component.errorString()));
		QScopedPointer<QObject> page(component.create());
		QVERIFY2(page, "GDownloadViewPage 实例化失败");
		auto* item = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(item);
		item->setParentItem(window.contentItem());
		item->setSize(QSizeF(window.width(), window.height()));
		QVERIFY(item->setProperty("pageType", 2));
		QVERIFY2(item->setProperty("model", QVariant::fromValue<QObject*>(&model)),
				 "设置 GDownloadViewPage.model 失败");
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QQuickItem* errorDetails = nullptr;
		QTRY_VERIFY_WITH_TIMEOUT((errorDetails = findVisualItem(item, QStringLiteral("taskErrorDetails"))),
							 1000);
		QVERIFY(errorDetails->property("visible").toBool());
		const QString details = errorDetails->property("text").toString();
		QVERIFY2(details.contains(QStringLiteral("3")), qPrintable(details));
		QVERIFY2(details.contains(QStringLiteral("Resource not found")), qPrintable(details));

		QQuickItem* retryButton = nullptr;
		QTRY_VERIFY_WITH_TIMEOUT((retryButton = findVisualItem(item, QStringLiteral("btnRetryTask"))), 1000);
		QVERIFY(retryButton->property("visible").toBool());
		QMetaObject::invokeMethod(retryButton, "clicked");

		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("RetryTask"));
		QCOMPARE(fakeBrowser_->lastRpcCall().args.size(), 1);
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(0).toString(), QStringLiteral("failed-1"));
	}

	void test_failed_task_without_original_link_hides_retry() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "failed-no-link"}, {"taskState", 4}, {"fileName", "broken.iso"},
			 {"savePath", "C:/Downloads/broken.iso"}, {"totalSize", "2.0 GB"},
			 {"currentSize", "0 B"}, {"downloadSpeed", "0 B/s"}, {"progress", 0},
			 {"remainingTime", "Stopped"}, {"connections", 0}, {"downloadLink", ""},
			 {"errorCode", "1"}, {"errorMessage", "Unknown error"}},
		});

		QQuickWindow window;
		window.resize(960, 260);
		window.show();

		QQmlComponent component(&engine_, QUrl("qrc:/qml/CommonComponents/GDownloadViewPage.qml"));
		QVERIFY2(!component.isError(), qPrintable(component.errorString()));
		QScopedPointer<QObject> page(component.create());
		QVERIFY(page);
		auto* item = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(item);
		item->setParentItem(window.contentItem());
		item->setSize(QSizeF(window.width(), window.height()));
		QVERIFY(item->setProperty("pageType", 2));
		QVERIFY(item->setProperty("model", QVariant::fromValue<QObject*>(&model)));
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QQuickItem* retryButton = nullptr;
		QTRY_VERIFY_WITH_TIMEOUT((retryButton = findVisualItem(item, QStringLiteral("btnRetryTask"))), 1000);
		QVERIFY(!retryButton->property("visible").toBool());
	}

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstDownloadFailure)
#include "tst_download_failure.moc"
