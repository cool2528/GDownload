#include <QtTest>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "TestStubs.h"

using namespace gdl::tests;

namespace {
QList<QQuickItem*> visualItems(QQuickItem* root, const QString& objectName) {
	QList<QQuickItem*> result;
	if (!root) return result;
	if (root->objectName() == objectName) result.append(root);
	for (QQuickItem* child : root->childItems()) result.append(visualItems(child, objectName));
	return result;
}

QRectF boundsIn(QQuickItem* item, QQuickItem* ancestor) {
	return {item->mapToItem(ancestor, QPointF(0, 0)), item->size()};
}

bool containsWithTolerance(const QRectF& outer, const QRectF& inner, qreal tolerance = 1.0) {
	return inner.left() >= outer.left() - tolerance && inner.top() >= outer.top() - tolerance &&
		   inner.right() <= outer.right() + tolerance && inner.bottom() <= outer.bottom() + tolerance;
}

QString readSource(const QString& relativePath) {
	QFile file(QStringLiteral("%1/%2").arg(QStringLiteral(SOURCE_ROOT), relativePath));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
	return QString::fromUtf8(file.readAll());
}

QList<QVariantMap> makeDownloadRows(int count) {
	QList<QVariantMap> rows;
	rows.reserve(count);
	for (int index = 0; index < count; ++index) {
		rows.append({{"taskId", QStringLiteral("large-%1").arg(index)},
					  {"taskState", 1},
					  {"fileName", QStringLiteral("Aurora-runtime-task-%1.iso").arg(index)},
					  {"savePath", QStringLiteral("C:/Downloads/Aurora-runtime-task-%1.iso").arg(index)},
					  {"totalSize", "4.0 GB"},
					  {"currentSize", "2.0 GB"},
					  {"downloadSpeed", "12.0 MB/s"},
					  {"progress", index % 100},
					  {"remainingTime", "3 min"},
					  {"connections", 8},
					  {"downloadLink", QStringLiteral("https://example.com/%1.iso").arg(index)}});
	}
	return rows;
}

QList<QVariantMap> makeCloudRows(int count) {
	QList<QVariantMap> rows;
	rows.reserve(count);
	for (int index = 0; index < count; ++index) {
		const bool directory = index % 9 == 0;
		rows.append({{"fileName", QStringLiteral("Cloud item %1").arg(index)},
					  {"filePath", QStringLiteral("/Aurora/runtime/%1").arg(index)},
					  {"fileId", QString::number(index)},
					  {"fileSize", directory ? QString() : QStringLiteral("128 MB")},
					  {"createTime", "2026-07-16 09:30"},
					  {"isDir", directory},
					  {"isSelected", false}});
	}
	return rows;
}
}  // namespace

class TstRuntimeBehavior : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_, &theme_, &netDisk_);
		engine_.rootContext()->setContextProperty("mainWindow", &mainWindowStub_);
	}

	void test_startup_and_page_switch_budget() {
		QQuickWindow window;
		window.resize(900, 640);
		window.show();

		QElapsedTimer startupTimer;
		startupTimer.start();
		QScopedPointer<QObject> page(createItem(QStringLiteral("qrc:/qml/Browser/BrowserView.qml"), window));
		QVERIFY(page);
		const qint64 startupMs = startupTimer.elapsed();
		qInfo() << "Aurora BrowserView cold creation (offscreen):" << startupMs << "ms";
		// Debug + offscreen 首次加载会包含 QML 类型编译；8 秒门槛用于捕获数量级回退，
		// 具体耗时始终写入日志，发布构建的基准采样另行人工执行。
		QVERIFY2(startupMs < 8000, qPrintable(QStringLiteral("BrowserView creation exceeded 8000 ms: %1 ms").arg(startupMs)));

		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		QCOMPARE(page->property("index").toInt(), 2);

		QElapsedTimer switchTimer;
		switchTimer.start();
		for (int iteration = 0; iteration < 120; ++iteration) {
			page->setProperty("index", iteration % 3);
			QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
		}
		const qint64 switchMs = switchTimer.elapsed();
		qInfo() << "Aurora 120 page switches (offscreen):" << switchMs << "ms";
		QVERIFY2(switchMs < 3000, qPrintable(QStringLiteral("120 page switches exceeded 3000 ms: %1 ms").arg(switchMs)));

		const QStringList workspaces{QStringLiteral("downloadWorkspace"), QStringLiteral("settingsWorkspace"),
									 QStringLiteral("homeWorkspace")};
		for (int selected = 0; selected < workspaces.size(); ++selected) {
			page->setProperty("index", selected);
			QCoreApplication::processEvents();
			for (int workspace = 0; workspace < workspaces.size(); ++workspace) {
				auto* item = page->findChild<QQuickItem*>(workspaces.at(workspace));
				QVERIFY2(item, qPrintable(QStringLiteral("Missing workspace %1").arg(workspaces.at(workspace))));
				QCOMPARE(item->isVisible(), workspace == selected);
			}
		}
	}

	void test_large_download_list_is_virtualized() {
		TestDownloadTaskModel model;
		model.setRows(makeDownloadRows(1200));

		QQuickWindow window;
		window.resize(900, 640);
		window.show();
		QScopedPointer<QObject> page(createItem(QStringLiteral("qrc:/qml/CommonComponents/GDownloadViewPage.qml"), window));
		QVERIFY(page);
		page->setProperty("pageType", 0);
		page->setProperty("model", QVariant::fromValue<QObject*>(&model));

		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		auto* list = page->findChild<QQuickItem*>(QStringLiteral("downloadLifecycleList"));
		QVERIFY(list);
		QTRY_COMPARE_WITH_TIMEOUT(list->property("count").toInt(), 1200, 3000);
		QTRY_VERIFY_WITH_TIMEOUT(!visualItems(root, QStringLiteral("downloadTaskCard")).isEmpty(), 3000);

		const int initialDelegates = visualItems(root, QStringLiteral("downloadTaskCard")).size();
		qInfo() << "Download delegates for 1200 rows:" << initialDelegates;
		QVERIFY2(initialDelegates < 80, qPrintable(QStringLiteral("ListView instantiated too many download delegates: %1").arg(initialDelegates)));
		QVERIFY(list->property("contentHeight").toReal() > list->height());

		list->setProperty("contentY", qMax<qreal>(0, list->property("contentHeight").toReal() - list->height()));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		QTRY_VERIFY_WITH_TIMEOUT(!visualItems(root, QStringLiteral("downloadTaskCard")).isEmpty(), 2000);
		const int endDelegates = visualItems(root, QStringLiteral("downloadTaskCard")).size();
		QVERIFY2(endDelegates < 80, qPrintable(QStringLiteral("Download delegate count grew after scrolling: %1").arg(endDelegates)));
	}

	void test_large_cloud_directory_is_virtualized_and_responsive() {
		netDisk_->setRows(makeCloudRows(1500));

		QQuickWindow window;
		window.resize(900, 640);
		window.show();
		QScopedPointer<QObject> page(createItem(QStringLiteral("qrc:/qml/CommonComponents/NetDiskPageView.qml"), window));
		QVERIFY(page);
		page->setProperty("parseMode", false);
		QCoreApplication::processEvents();

		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		auto* list = page->findChild<QQuickItem*>(QStringLiteral("netDiskFileList"));
		QVERIFY(list);
		QTRY_COMPARE_WITH_TIMEOUT(list->property("count").toInt(), 1500, 3000);
		QTRY_VERIFY_WITH_TIMEOUT(!visualItems(root, QStringLiteral("netDiskFileRow")).isEmpty(), 3000);
		const int initialDelegates = visualItems(root, QStringLiteral("netDiskFileRow")).size();
		qInfo() << "Cloud delegates for 1500 rows:" << initialDelegates;
		QVERIFY2(initialDelegates < 80, qPrintable(QStringLiteral("ListView instantiated too many cloud delegates: %1").arg(initialDelegates)));

		window.resize(440, 540);
		root->setSize(QSizeF(window.width(), window.height()));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		QTRY_VERIFY_WITH_TIMEOUT(page->property("compact").toBool(), 1000);
		QTRY_VERIFY_WITH_TIMEOUT(page->property("veryCompact").toBool(), 1000);
		for (QQuickItem* row : visualItems(root, QStringLiteral("netDiskFileRow"))) {
			if (!row->isVisible()) continue;
			// ListView 会在视口上下保留少量预取委托；垂直越界由 clip 负责，
			// 这里验证窄宽下行不会横向溢出工作区。
			const QRectF rowBounds = boundsIn(row, root);
			QVERIFY(rowBounds.left() >= -1.0);
			QVERIFY(rowBounds.right() <= root->width() + 1.0);
		}

		list->setProperty("contentY", qMax<qreal>(0, list->property("contentHeight").toReal() - list->height()));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		QVERIFY(visualItems(root, QStringLiteral("netDiskFileRow")).size() < 80);
	}

	void test_theme_switch_updates_live_surfaces_without_stale_colors() {
		QQuickWindow window;
		window.resize(900, 640);
		window.show();
		QScopedPointer<QObject> browser(createItem(QStringLiteral("qrc:/qml/Browser/BrowserView.qml"), window));
		QVERIFY(browser);
		auto* browserItem = qobject_cast<QQuickItem*>(browser.data());
		QVERIFY(browserItem);

		QScopedPointer<QObject> cloud(createItem(QStringLiteral("qrc:/qml/CommonComponents/NetDiskPageView.qml"), window));
		QVERIFY(cloud);
		const QColor lightColor = browser->property("color").value<QColor>();
		QCOMPARE(lightColor, theme_->property("bgPage").value<QColor>());
		QCOMPARE(cloud->property("color").value<QColor>(), lightColor);

		QVERIFY(theme_->setProperty("theme", 2));
		QTRY_COMPARE_WITH_TIMEOUT(browser->property("color").value<QColor>(), theme_->property("bgPage").value<QColor>(), 1000);
		QTRY_COMPARE_WITH_TIMEOUT(cloud->property("color").value<QColor>(), theme_->property("bgPage").value<QColor>(), 1000);
		QVERIFY(browser->property("color").value<QColor>() != lightColor);

		QVERIFY(theme_->setProperty("theme", 1));
		QTRY_COMPARE_WITH_TIMEOUT(browser->property("color").value<QColor>(), lightColor, 1000);
		QTRY_COMPARE_WITH_TIMEOUT(cloud->property("color").value<QColor>(), lightColor, 1000);
	}

	void test_resize_minimum_and_overdraw_contracts() {
		const QString mainWindow = readSource(QStringLiteral("src/App/ui/Resource/qml/mainWindow.qml"));
		const QString browserView = readSource(QStringLiteral("src/App/ui/Resource/qml/Browser/BrowserView.qml"));
		const QString downloadView = readSource(QStringLiteral("src/App/ui/Resource/qml/Browser/DownloadPageView.qml"));
		const QString lifecycle = readSource(QStringLiteral("src/App/ui/Resource/qml/CommonComponents/GDownloadViewPage.qml"));
		const QString cloud = readSource(QStringLiteral("src/App/ui/Resource/qml/CommonComponents/NetDiskPageView.qml"));
		QVERIFY(!mainWindow.isEmpty());
		QVERIFY(!browserView.isEmpty());
		QVERIFY(!downloadView.isEmpty());
		QVERIFY(!lifecycle.isEmpty());
		QVERIFY(!cloud.isEmpty());

		QVERIFY(mainWindow.contains(QRegularExpression(QStringLiteral(R"(minimumWidth\s*:\s*900\b)"))));
		QVERIFY(mainWindow.contains(QRegularExpression(QStringLiteral(R"(minimumHeight\s*:\s*640\b)"))));
		QVERIFY(mainWindow.contains(QStringLiteral("SplitView.minimumWidth: 600")));
		QCOMPARE(downloadView.count(QRegularExpression(QStringLiteral(R"(visible\s*:\s*control\.currentIndex\s*===\s*[012])"))), 3);
		QVERIFY(!downloadView.contains(QRegularExpression(QStringLiteral(R"(opacity\s*:\s*control\.currentIndex)"))));
		QVERIFY(lifecycle.contains(QStringLiteral("ListView")) && lifecycle.contains(QStringLiteral("clip: true")));
		QVERIFY(cloud.contains(QStringLiteral("ListView")) && cloud.contains(QStringLiteral("clip: true")));

		const QRegularExpression rawDuration(QStringLiteral(R"(duration\s*:\s*\d+)"));
		for (const auto& source : {mainWindow, browserView, downloadView, lifecycle, cloud}) {
			QVERIFY2(!source.contains(rawDuration), "Runtime surfaces must use theme motion tokens, not raw animation durations");
		}

		QQuickWindow window;
		window.resize(826, 600);
		window.show();
		QScopedPointer<QObject> page(createItem(QStringLiteral("qrc:/qml/Browser/DownloadPageView.qml"), window));
		QVERIFY(page);
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		auto* card = page->findChild<QQuickItem*>(QStringLiteral("downloadSummaryCard"));
		auto* grid = page->findChild<QQuickItem*>(QStringLiteral("downloadSummaryGrid"));
		QVERIFY(card);
		QVERIFY(grid);
		const QStringList lifecyclePages{QStringLiteral("downloadPage"), QStringLiteral("waitingPage"),
									 QStringLiteral("completedPage")};
		for (int selected = 0; selected < lifecyclePages.size(); ++selected) {
			page->setProperty("currentIndex", selected);
			QCoreApplication::processEvents();
			for (int lifecycleIndex = 0; lifecycleIndex < lifecyclePages.size(); ++lifecycleIndex) {
				auto* lifecyclePage = page->findChild<QQuickItem*>(lifecyclePages.at(lifecycleIndex));
				QVERIFY(lifecyclePage);
				QCOMPARE(lifecyclePage->isVisible(), lifecycleIndex == selected);
			}
		}
		QTRY_COMPARE_WITH_TIMEOUT(grid->property("columns").toInt(), 4, 1000);
		QVERIFY(containsWithTolerance(QRectF(QPointF(0, 0), root->size()), boundsIn(card, root)));

		window.resize(640, 520);
		root->setSize(QSizeF(window.width(), window.height()));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		QTRY_COMPARE_WITH_TIMEOUT(grid->property("columns").toInt(), 2, 1000);
		QVERIFY(containsWithTolerance(QRectF(QPointF(0, 0), root->size()), boundsIn(card, root)));
	}

   private:
	QObject* createItem(const QString& url, QQuickWindow& window) {
		QQmlComponent component(&engine_, QUrl(url));
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
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		return object;
	}

	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
	TestGTheme* theme_ = nullptr;
	TestNetWorkDiskManager* netDisk_ = nullptr;
	QObject mainWindowStub_;
};

QTEST_MAIN(TstRuntimeBehavior)
#include "tst_runtime_behavior.moc"
