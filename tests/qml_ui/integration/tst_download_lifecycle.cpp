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
void collectVisualItems(QQuickItem* root, const QString& objectName, QList<QQuickItem*>& matches) {
	if (!root) return;
	if (root->objectName() == objectName) matches.append(root);
	for (QQuickItem* child : root->childItems()) collectVisualItems(child, objectName, matches);
}

QList<QQuickItem*> findVisualItems(QQuickItem* root, const QString& objectName) {
	QList<QQuickItem*> matches;
	collectVisualItems(root, objectName, matches);
	return matches;
}

QQuickItem* findVisibleItem(QQuickItem* root, const QString& objectName) {
	for (QQuickItem* item : findVisualItems(root, objectName)) {
		if (item->isVisible()) return item;
	}
	return nullptr;
}

QRectF boundsIn(QQuickItem* item, QQuickItem* ancestor) {
	return {item->mapToItem(ancestor, QPointF(0, 0)), item->size()};
}

bool containsWithTolerance(const QRectF& outer, const QRectF& inner, qreal tolerance = 1.0) {
	return inner.left() >= outer.left() - tolerance && inner.top() >= outer.top() - tolerance &&
		   inner.right() <= outer.right() + tolerance && inner.bottom() <= outer.bottom() + tolerance;
}
}  // namespace

class TstDownloadLifecycle : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
	}

	void init() { fakeBrowser_->clearHistory(); }

	void test_active_row_exposes_transfer_metadata_and_pause() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "active-1"}, {"taskState", 1}, {"fileName", "Aurora-Linux.iso"},
			 {"savePath", "C:/Downloads/Aurora-Linux.iso"}, {"totalSize", "4.0 GB"},
			 {"currentSize", "2.0 GB"}, {"downloadSpeed", "12.0 MB/s"}, {"progress", 50},
			 {"remainingTime", "3 min"}, {"connections", 8},
			 {"downloadLink", "https://example.com/aurora.iso"}},
		});

		QQuickWindow window;
		window.resize(760, 360);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 0, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QVERIFY(findVisibleItem(root, QStringLiteral("taskTransferProgress")));
		QVERIFY(findVisibleItem(root, QStringLiteral("activeSpeedMetadata")));
		QVERIFY(findVisibleItem(root, QStringLiteral("activeEtaMetadata")));
		QQuickItem* pause = findVisibleItem(root, QStringLiteral("btnPauseTask"));
		QVERIFY(pause);
		QMetaObject::invokeMethod(pause, "clicked");
		QTRY_COMPARE(fakeBrowser_->lastRpcMethod(), QStringLiteral("PauseTask"));
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(0).toInt(), 0);
		QCOMPARE(fakeBrowser_->lastRpcCall().args.at(1).toString(), QStringLiteral("active-1"));
	}

	void test_waiting_row_has_queue_metadata_without_transfer_progress() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "waiting-1"}, {"taskState", 3}, {"fileName", "Qt-Offline.7z"},
			 {"savePath", "C:/Downloads/Qt-Offline.7z"}, {"totalSize", "640 MB"},
			 {"currentSize", "0 B"}, {"downloadSpeed", "0 B/s"}, {"progress", 0},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"downloadLink", "https://example.com/qt.7z"}},
		});

		QQuickWindow window;
		window.resize(760, 340);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 1, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		const auto progressItems = findVisualItems(root, QStringLiteral("taskTransferProgress"));
		QCOMPARE(progressItems.size(), 1);
		QVERIFY(!progressItems.first()->isVisible());
		QVERIFY(findVisibleItem(root, QStringLiteral("waitingQueuePosition")));
		QVERIFY(findVisibleItem(root, QStringLiteral("waitingExpectedSize")));
		QVERIFY(findVisibleItem(root, QStringLiteral("btnStartWaitingTask")));
		QVERIFY(findVisibleItem(root, QStringLiteral("btnRemoveWaitingTask")));
		QVERIFY(!findVisibleItem(root, QStringLiteral("activeSpeedMetadata")));
		QVERIFY(!findVisibleItem(root, QStringLiteral("activeEtaMetadata")));
	}

	void test_stopped_rows_are_grouped_as_completed_and_failed() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "complete-1"}, {"taskState", 0}, {"fileName", "GDownload.exe"},
			 {"savePath", "C:/Downloads/GDownload.exe"}, {"totalSize", "86 MB"},
			 {"currentSize", "86 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 100},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"downloadLink", "https://example.com/GDownload.exe"}, {"errorCode", ""},
			 {"errorMessage", ""}},
			{{"taskId", "failed-1"}, {"taskState", 4}, {"fileName", "Broken.iso"},
			 {"savePath", "C:/Downloads/Broken.iso"}, {"totalSize", "2.4 GB"},
			 {"currentSize", "312 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 13},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"downloadLink", "https://example.com/Broken.iso"}, {"errorCode", "3"},
			 {"errorMessage", "Resource not found"}},
		});

		QQuickWindow window;
		window.resize(820, 620);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 2, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 2, 1000);

		QVERIFY(findVisibleItem(root, QStringLiteral("completedSectionHeader")));
		QVERIFY(findVisibleItem(root, QStringLiteral("failedSectionHeader")));
		QVERIFY(findVisibleItem(root, QStringLiteral("completedSizeMetadata")));
		QVERIFY(findVisibleItem(root, QStringLiteral("taskErrorDetails")));
		QVERIFY(findVisibleItem(root, QStringLiteral("btnOpenCompletedTask")));
		QVERIFY(findVisibleItem(root, QStringLiteral("btnRetryTask")));
		for (QQuickItem* progress : findVisualItems(root, QStringLiteral("taskTransferProgress"))) {
			QVERIFY(!progress->isVisible());
		}

		for (QQuickItem* card : findVisualItems(root, QStringLiteral("downloadTaskCard"))) {
			if (!card->isVisible()) continue;
			QQuickItem* actions = findVisibleItem(card, QStringLiteral("taskActionFlow"));
			QVERIFY(actions);
			const QRectF actionBounds = boundsIn(actions, card);
			QVERIFY2(actionBounds.right() >= card->width() - 190.0,
			         "Download actions must stay in the fixed trailing column");
			QVERIFY2(actionBounds.top() < card->height() * 0.5,
			         "Download actions must remain in the card header, not below the error body");
		}
	}

	// 回归:一次成功的下载不能在"Completed size"旁边再摆一个更小的数字。ed2k 续传完成时
	// 曾出现"完成大小 3.23 MB / 已传输 2.70 MB"(后者只是本轮传输量),用户据此判定下载失败。
	void test_completed_row_hides_redundant_downloaded_chip() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "complete-full"}, {"taskState", 0}, {"fileName", "emule-windows.exe"},
			 {"savePath", "C:/Downloads/emule-windows.exe"}, {"totalSize", "3.23 MB"},
			 {"currentSize", "3.23 MB"}, {"downloadSpeed", "0 B/s"}, {"progress", 100},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"totalSizeBytes", QVariant::fromValue<qint64>(3389035)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(3389035)},
			 {"downloadLink", "ed2k://|file|emule-windows.exe|3389035|3d366ed505b977fc61c9a6ee01e96329|/"},
			 {"errorCode", ""}, {"errorMessage", ""}},
		});

		QQuickWindow window;
		window.resize(820, 400);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 2, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QVERIFY(findVisibleItem(root, QStringLiteral("completedSizeMetadata")));
		QVERIFY2(!findVisibleItem(root, QStringLiteral("completedDownloadedMetadata")),
		         "A fully downloaded task must not show a second, redundant size chip");
	}

	// 反向用例:确实只拿到一部分(例如 BT 用 select-file 只下选中的文件)时,这条必须还在,
	// 否则"完成"会掩盖掉"只下了一部分"这个事实。
	void test_completed_row_keeps_downloaded_chip_when_partial() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "complete-partial"}, {"taskState", 0}, {"fileName", "Bundle.torrent"},
			 {"savePath", "C:/Downloads/Bundle"}, {"totalSize", "10.00 GB"},
			 {"currentSize", "2.00 GB"}, {"downloadSpeed", "0 B/s"}, {"progress", 20},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"totalSizeBytes", QVariant::fromValue<qint64>(10737418240LL)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(2147483648LL)},
			 {"downloadLink", "magnet:?xt=urn:btih:0123456789abcdef"}, {"errorCode", ""},
			 {"errorMessage", ""}},
		});

		QQuickWindow window;
		window.resize(820, 400);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 2, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QVERIFY(findVisibleItem(root, QStringLiteral("completedSizeMetadata")));
		QVERIFY(findVisibleItem(root, QStringLiteral("completedDownloadedMetadata")));
	}

	// 回归:可见性判据不能拿格式化字符串比大小。FormatFileSize 只留两位小数,10 GB 量级上
	// 5 MB 的缺口会被舍进同一个字符串("10.00 GB" == "10.00 GB"),于是一个真的没下完的
	// 任务被当成完整的,"Downloaded"这条唯一能暴露缺口的信息被藏掉。必须比较原始字节数。
	void test_completed_row_keeps_downloaded_chip_when_rounding_hides_the_gap() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "complete-rounded"}, {"taskState", 0}, {"fileName", "Archive.bin"},
			 {"savePath", "C:/Downloads/Archive.bin"}, {"totalSize", "10.00 GB"},
			 {"currentSize", "10.00 GB"}, {"downloadSpeed", "0 B/s"}, {"progress", 99},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 // 差 5 MB,格式化后两串完全相同
			 {"totalSizeBytes", QVariant::fromValue<qint64>(10737418240LL)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(10732175360LL)},
			 {"downloadLink", "https://example.com/Archive.bin"}, {"errorCode", ""},
			 {"errorMessage", ""}},
		});

		QQuickWindow window;
		window.resize(820, 400);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 2, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QVERIFY2(findVisibleItem(root, QStringLiteral("completedDownloadedMetadata")),
		         "A rounding-equal but byte-unequal task must still show the Downloaded chip");
	}

	// 回归:kRemoved(5) 是"用户取消/被移除",不是"已完成"。旧判据 completedTask =
	// stoppedTask && !failedTask 把它一并算成完成,于是一个取消掉的任务顶着绿色
	// "Completed" 徽标和一个"Open"按钮 —— 而那个文件根本不完整,点开只会打开半个文件。
	void test_cancelled_row_is_not_presented_as_completed() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "cancelled-1"}, {"taskState", 5}, {"fileName", "Cancelled.iso"},
			 {"savePath", "C:/Downloads/Cancelled.iso"}, {"totalSize", "2.00 GB"},
			 {"currentSize", "0.30 GB"}, {"downloadSpeed", "0 B/s"}, {"progress", 15},
			 {"remainingTime", "Unknown"}, {"connections", 0},
			 {"totalSizeBytes", QVariant::fromValue<qint64>(2147483648LL)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(322122547LL)},
			 {"downloadLink", "https://example.com/Cancelled.iso"}, {"errorCode", ""},
			 {"errorMessage", ""}},
		});

		QQuickWindow window;
		window.resize(820, 400);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 2, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QQuickItem* card = findVisibleItem(root, QStringLiteral("downloadTaskCard"));
		QVERIFY(card);
		QVERIFY2(!card->property("completedTask").toBool(),
		         "A cancelled task must not be classified as completed");
		QVERIFY2(!findVisibleItem(root, QStringLiteral("btnOpenCompletedTask")),
		         "A cancelled task must not offer to open its incomplete file");
	}

	// 引擎的 speed 现在是"线上到达的字节",这些字节可能永远落不了盘(未凑齐的块被丢弃、
	// part 校验失败整段重下、间隙帧被弃)。此时界面上是"Speed 96 KB/s"配绿色成功样式,
	// 而进度一动不动 —— 比原来的"Speed 0 B"更具误导性。必须有一句明确的告警。
	void test_active_row_warns_when_data_arrives_but_nothing_lands() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "stalled-1"}, {"taskState", 1}, {"fileName", "Half-dead.rar"},
			 {"savePath", "C:/Downloads/Half-dead.rar"}, {"totalSize", "700.00 MB"},
			 {"currentSize", "12.00 MB"}, {"downloadSpeed", "96.00 KB"}, {"progress", 2},
			 {"remainingTime", "Unknown"}, {"connections", 3},
			 {"totalSizeBytes", QVariant::fromValue<qint64>(734003200LL)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(12582912LL)},
			 {"progressStalled", true},
			 {"downloadLink", "ed2k://|file|Half-dead.rar|734003200|3d366ed505b977fc61c9a6ee01e96329|/"}},
		});

		QQuickWindow window;
		window.resize(820, 460);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 0, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QQuickItem* warning = findVisibleItem(root, QStringLiteral("taskStallWarning"));
		QVERIFY2(warning, "A transfer that receives data but saves none must say so");
		const QString text = warning->property("text").toString();
		QVERIFY2(text.contains(QStringLiteral("saved"), Qt::CaseInsensitive),
		         qPrintable(QStringLiteral("Stall warning must say nothing is being saved, got: %1").arg(text)));
	}

	// 反向用例:进度在正常推进时,绝不能弹出停滞告警,否则告警会被当成噪音无视掉。
	void test_active_row_has_no_stall_warning_while_progress_advances() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "healthy-1"}, {"taskState", 1}, {"fileName", "Healthy.rar"},
			 {"savePath", "C:/Downloads/Healthy.rar"}, {"totalSize", "700.00 MB"},
			 {"currentSize", "120.00 MB"}, {"downloadSpeed", "96.00 KB"}, {"progress", 17},
			 {"remainingTime", "1 h 40 m"}, {"connections", 3},
			 {"totalSizeBytes", QVariant::fromValue<qint64>(734003200LL)},
			 {"currentSizeBytes", QVariant::fromValue<qint64>(125829120LL)},
			 {"progressStalled", false},
			 {"downloadLink", "ed2k://|file|Healthy.rar|734003200|3d366ed505b977fc61c9a6ee01e96329|/"}},
		});

		QQuickWindow window;
		window.resize(820, 460);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 0, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);

		QVERIFY(!findVisibleItem(root, QStringLiteral("taskStallWarning")));
	}

	void test_narrow_row_contains_long_name_and_all_visible_actions() {
		TestDownloadTaskModel model;
		model.setRows({
			{{"taskId", "narrow-1"}, {"taskState", 1},
			 {"fileName", "This-is-a-very-long-download-file-name-that-must-not-cover-actions-or-leave-the-card.iso"},
			 {"savePath", "C:/Downloads/long.iso"}, {"totalSize", "8.0 GB"},
			 {"currentSize", "4.0 GB"}, {"downloadSpeed", "20.0 MB/s"}, {"progress", 50},
			 {"remainingTime", "4 min"}, {"connections", 16},
			 {"downloadLink", "https://example.com/long.iso"}},
		});

		QQuickWindow window;
		window.resize(420, 520);
		window.show();
		QScopedPointer<QObject> page(createPage(window, 0, model));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QTRY_COMPARE_WITH_TIMEOUT(page->property("taskCount").toInt(), 1, 1000);
		QQuickItem* card = findVisibleItem(root, QStringLiteral("downloadTaskCard"));
		QVERIFY(card);
		QTRY_VERIFY_WITH_TIMEOUT(card->height() > 0, 1000);

		const QRectF cardBounds(QPointF(0, 0), card->size());
		for (const QString& objectName : {QStringLiteral("taskFileName"),
									 QStringLiteral("taskActionFlow"), QStringLiteral("btnPauseTask"),
									 QStringLiteral("btnOpenTaskLocation"), QStringLiteral("btnCopyTaskLink"),
									 QStringLiteral("btnDeleteTask")}) {
			QQuickItem* item = findVisibleItem(root, objectName);
			QVERIFY2(item, qPrintable(QStringLiteral("Missing visible item: %1").arg(objectName)));
			const QRectF itemBounds = boundsIn(item, card);
			QVERIFY2(containsWithTolerance(cardBounds, itemBounds),
					 qPrintable(QStringLiteral("%1 exceeds task card: card=%2,%3 %4x%5 item=%6,%7 %8x%9")
								.arg(objectName)
								.arg(cardBounds.x()).arg(cardBounds.y()).arg(cardBounds.width()).arg(cardBounds.height())
								.arg(itemBounds.x()).arg(itemBounds.y()).arg(itemBounds.width()).arg(itemBounds.height())));
		}
	}

	void test_narrow_title_actions_remain_inside_header() {
		QQuickWindow window;
		window.resize(420, 180);
		window.show();

		QQmlComponent component(&engine_, QUrl("qrc:/qml/Browser/DownloadPageTitle.qml"));
		QVERIFY2(!component.isError(), qPrintable(component.errorString()));
		QScopedPointer<QObject> title(component.create());
		QVERIFY(title);
		auto* root = qobject_cast<QQuickItem*>(title.data());
		QVERIFY(root);
		root->setParentItem(window.contentItem());
		root->setSize(QSizeF(window.width(), 116));
		QTRY_VERIFY_WITH_TIMEOUT(root->height() > 0, 1000);

		QQuickItem* actions = findVisibleItem(root, QStringLiteral("downloadPageActions"));
		QVERIFY(actions);
		QVERIFY(containsWithTolerance(QRectF(QPointF(0, 0), root->size()), boundsIn(actions, root)));
		for (const QString& objectName : {QStringLiteral("btnAddDownload"),
									 QStringLiteral("btnStartAllTasks"), QStringLiteral("btnPauseAllTasks"),
									 QStringLiteral("btnRefreshTasks"), QStringLiteral("btnDeleteAllTasks")}) {
			QQuickItem* action = findVisibleItem(root, objectName);
			QVERIFY2(action, qPrintable(QStringLiteral("Missing title action: %1").arg(objectName)));
			QVERIFY2(containsWithTolerance(QRectF(QPointF(0, 0), root->size()), boundsIn(action, root)),
					 qPrintable(QStringLiteral("Title action exceeds header: %1").arg(objectName)));
		}
	}

   private:
	QObject* createPage(QQuickWindow& window, int pageType, TestDownloadTaskModel& model) {
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
		item->setProperty("model", QVariant::fromValue<QObject*>(&model));
		QCoreApplication::processEvents();
		return object;
	}

	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstDownloadLifecycle)
#include "tst_download_lifecycle.moc"
