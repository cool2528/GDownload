#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"

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
		setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
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

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstSaveSettings)
#include "tst_save_settings.moc"
