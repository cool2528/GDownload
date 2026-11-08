#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJSValue>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"
#include "TestStubs.h"

namespace {
QList<QQuickItem*> visualItems(QQuickItem* root, const QString& name) {
	QList<QQuickItem*> result;
	if (root->objectName() == name) result.append(root);
	for (auto* child : root->childItems()) result.append(visualItems(child, name));
	return result;
}

QJsonArray trackerNames(const QString& json) {
	return QJsonDocument::fromJson(json.toUtf8()).array();
}
}  // namespace

using namespace gdl::tests;

// tst_save_settings:验证设置页 Save 写入 / Reset 不写入
//
// 加载 ConnectionPerformanceSettingPage(暂存-保存派,内嵌 SettingFormActions):
//   - test_save_writes_settings: 改 SpinBox(maxConcurrentDownloadsInput)触发 hasChanges,
//     点 btnSave → onSave → applySettings → SetAria2MaxConcurrentDownloads(新值),
//     断言 FakeSettingsManager.writeHistory 含 MaxConcurrentDownloads
//   - test_cancel_does_not_write: 改 SpinBox 后点 btnCancel(Reset)→ onReset 仅重置输入,
//     不调 setter,断言 writeHistory 为空
//
// 注:spec/brief 原假设 BasicSettingPage 有 btnSave,但实际 BasicSettingPage 为即时提交派
// (无统一保存按钮),故改用 ConnectionPerformanceSettingPage(暂存-保存派,含 btnSave/btnCancel)。
class TstSaveSettings : public QObject {
	Q_OBJECT

   private slots:
	void initTestCase() {
		qputenv("GDOWNLOAD_TEST", "1");
		fakeBrowser_ = new FakeBrowserManager(this);
		fakeSettings_ = new FakeSettingsManager(this);
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_, &theme_);
	}

	void init() {
		fakeSettings_->clearHistory();
		fakeBrowser_->clearHistory();
	}

	void test_save_writes_settings() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/Browser/ConnectionPerformanceSettingPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create());
		QVERIFY2(!page.isNull(), "页面实例化失败");

		// 找到并行下载数 SpinBox,改值触发 hasChanges(默认 5 → 10)
		auto* spinbox = page->findChild<QObject*>("maxConcurrentDownloadsInput");
		QVERIFY2(spinbox, "未找到 maxConcurrentDownloadsInput");
		QVERIFY2(spinbox->setProperty("value", 10), "设置 SpinBox value 失败");
		QCOMPARE(spinbox->property("value").toInt(), 10);

		// 点 btnSave → onSave → applySettings → SetAria2MaxConcurrentDownloads(10)
		auto* btnSave = page->findChild<QQuickItem*>("btnSave");
		QVERIFY2(btnSave, "未找到 btnSave");
		// hasChanges=true 时 btnSave 应启用
		QVERIFY2(btnSave->property("enabled").toBool(), "btnSave 应在 hasChanges 时启用");
		QMetaObject::invokeMethod(btnSave, "clicked");

		// 断言写入历史非空,且含 MaxConcurrentDownloads
		QTRY_VERIFY_WITH_TIMEOUT(fakeSettings_->writeCount() > 0, 1000);
		bool found = false;
		for (const auto& w : fakeSettings_->writeHistory()) {
			if (w.key == QStringLiteral("MaxConcurrentDownloads")) {
				found = true;
				break;
			}
		}
		QVERIFY2(found, "writeHistory 应含 MaxConcurrentDownloads");
	}

	void test_cancel_does_not_write() {
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/Browser/ConnectionPerformanceSettingPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create());
		QVERIFY2(!page.isNull(), "页面实例化失败");

		auto* spinbox = page->findChild<QObject*>("maxConcurrentDownloadsInput");
		QVERIFY2(spinbox, "未找到 maxConcurrentDownloadsInput");
		QVERIFY2(spinbox->setProperty("value", 10), "设置 SpinBox value 失败");

		// 点 btnCancel(Reset)→ onReset 仅重置输入控件为默认值,不调 setter
		auto* btnCancel = page->findChild<QQuickItem*>("btnCancel");
		QVERIFY2(btnCancel, "未找到 btnCancel");
		QMetaObject::invokeMethod(btnCancel, "clicked");

		// 断言无任何 setter 写入
		QVERIFY2(fakeSettings_->writeHistory().isEmpty(),
				 "Reset 不应触发任何 setter 写入");
	}

	void test_tracker_remove_data() {
		QTest::addColumn<int>("pageWidth");
		QTest::addColumn<int>("themeMode");
		for (const int width : {900, 420, 320}) {
			for (const int mode : {1, 2}) {
				const auto tag = QStringLiteral("%1-%2").arg(width).arg(mode == 1 ? "light" : "dark");
				QTest::newRow(qPrintable(tag)) << width << mode;
			}
		}
	}

	void test_tracker_remove() {
		QFETCH(int, pageWidth);
		QFETCH(int, themeMode);
		theme_->Settheme(themeMode);
		const QString longUrl = QStringLiteral("https://example.com/") + QString(1600, QLatin1Char('x')) +
			QStringLiteral("/trackers.txt?list=all");
		const QString shortUrl = QStringLiteral("https://111");
		const QJsonArray initial{QStringLiteral("ngosang-best-link"), QStringLiteral("XIU2-all"), longUrl, shortUrl};
		fakeSettings_->SetTrackerSourceNames(QString::fromUtf8(QJsonDocument(initial).toJson(QJsonDocument::Compact)));
		fakeSettings_->clearHistory();

		QQuickWindow window;
		window.resize(pageWidth, 1800);
		window.show();
		QQmlComponent comp(&engine_, QUrl("qrc:/qml/Browser/TrackerServerSettingPage.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> page(comp.create());
		auto* root = qobject_cast<QQuickItem*>(page.data());
		QVERIFY(root);
		root->setParentItem(window.contentItem());
		root->setSize(QSizeF(pageWidth, root->implicitHeight()));
		QTRY_COMPARE(visualItems(root, QStringLiteral("customTrackerSourceRow")).size(), 2);
		QTest::qWait(250);
		QCOMPARE(fakeSettings_->writeCount(), 0);

		const auto rows = visualItems(root, QStringLiteral("customTrackerSourceRow"));
		for (auto* row : rows) {
			auto* button = visualItems(row, QStringLiteral("removeCustomTrackerSourceButton")).value(0);
			auto* label = visualItems(row, QStringLiteral("customTrackerSourceUrl")).value(0);
			QVERIFY(button);
			QVERIFY(label);
			QVERIFY(button->isVisible() && button->isEnabled());
			QVERIFY(button->width() >= button->implicitWidth());
			QVERIFY(button->height() > 0);
			const QPointF position = button->mapToItem(root, QPointF());
			QVERIFY2(position.x() >= 0 && position.x() + button->width() <= root->width(),
					 "Remove must remain inside the card at narrow widths");
			QVERIFY(label->x() + label->width() <= button->x());
			QCOMPARE(button->property("variant").toString(), QStringLiteral("default"));
			auto* background = button->property("background").value<QQuickItem*>();
			QVERIFY(background);
			QCOMPARE(background->property("color").value<QColor>(), theme_->dangerColor());
			QCOMPARE(button->property("contentColor").value<QColor>(), theme_->textInverse());
			if (row->property("modelData").toString() == longUrl) {
				QVERIFY(label->implicitWidth() > label->width());
				QVERIFY(label->property("truncated").toBool());
			}
		}

		// 使用实际指针点击,同时覆盖命中区域与即时提交,而非直接发 clicked 信号。
		auto* remove = visualItems(rows.first(), QStringLiteral("removeCustomTrackerSourceButton")).first();
		const QPoint click = remove->mapToScene(QPointF(remove->width() / 2, remove->height() / 2)).toPoint();
		QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, click);
		QCOMPARE(fakeSettings_->writeCount(), 1);
		QCOMPARE(fakeSettings_->lastWrittenKey(), QStringLiteral("TrackerSourceNames"));
		const QJsonArray expected{QStringLiteral("ngosang-best"), QStringLiteral("XIU2-all"), shortUrl};
		QCOMPARE(trackerNames(fakeSettings_->GetTrackerSourceNames()), expected);
		QTRY_COMPARE(visualItems(root, QStringLiteral("customTrackerSourceRow")).size(), 1);
		QCOMPARE(visualItems(root, QStringLiteral("customTrackerSourceRow")).first()->property("modelData").toString(), shortUrl);

		// 重建页面读回提交值,确认删除不依赖页面内临时数组,其余源保持不变。
		page.reset();
		QScopedPointer<QObject> reopened(comp.create());
		QVERIFY(reopened);
		QCOMPARE(reopened->property("customSources").value<QJSValue>().toVariant().toList(), QVariantList{shortUrl});
		QCOMPARE(fakeSettings_->writeCount(), 1);
	}

   private:
	TestGTheme* theme_ = nullptr;
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstSaveSettings)
#include "tst_save_settings.moc"
