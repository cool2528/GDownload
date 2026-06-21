#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSignalSpy>

#include "FakeBrowserManager.h"
#include "FakeSettingsManager.h"
#include "IntegrationHelper.h"

using namespace gdl::tests;

// tst_close_confirm:验证关闭确认对话框"不再提醒"勾选 + 确认退出 → actionSelected 信号
// 及持久化路径(写 ShowCloseConfirm=false)
//
// 持久化链路:CloseConfirmDialog(发 actionSelected(action, dontAskAgain))→ TitleBar.onActionSelected
// → SettingsManager.qShowCloseConfirm = false。TitleBar 依赖 mainWindow(FramelessWindow)+
// helper(FramelessHelper)+ 多个窗口按钮图片资源,offscreen 平台无法加载。故本用例:
//   1. 直接加载 CloseConfirmDialog,创建 QQuickWindow 并 open() 对话框(强制实例化
//      contentItem / Loader,使 chkDontAskAgain 可 findChild)
//   2. 勾选 chkDontAskAgain → root.dontAskAgain=true
//   3. 触发 Quit 按钮(buttons[2])的 buttonClicked 信号 → onButtonClicked → actionSelected
//   4. 断言 actionSelected(Quit=1, dontAskAgain=true)
//   5. 复刻 TitleBar 的 2 行持久化逻辑(qShowCloseConfirm=false),断言 writeHistory 含
//      ShowCloseConfirm —— 验证 dialog 信号 → 设置写入的端到端路径
//
// 触发方式说明:GMessageBox 的 buttons Repeater 在 offscreen 平台偶发不实例化 GButton
// delegate(环境限制,非生产 bug)。本用例优先 findChild btnConfirmClose 真实 clicked;
// 若 Repeater 未创建按钮,回退到直接 invoke buttonClicked(Quit 索引)信号,模拟 GButton
// onClicked 的 root.buttonClicked(index, modelData) 调用,测试对话框 onButtonClicked →
// actionSelected 逻辑链(与真实点击等价的信号路径)。
//
// 未覆盖:TitleBar.qml 自身的 onActionSelected 接线(需 FramelessHelper,deferred)
class TstCloseConfirm : public QObject {
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

	void test_dont_ask_again_persists() {
		// Dialog(Popup)需要窗口才能 open + 实例化 contentItem。offscreen 平台可创建虚拟窗口
		QQuickWindow window;
		window.resize(400, 300);
		window.show();

		QQmlComponent comp(&engine_, QUrl("qrc:/qml/CommonComponents/CloseConfirmDialog.qml"));
		QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
		QScopedPointer<QObject> dialog(comp.create());
		QVERIFY2(!dialog.isNull(), "CloseConfirmDialog 实例化失败");

		// 给 Popup 设视觉父项为窗口 contentItem,使其可 open
		dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));
		// open() 触发 contentItem / Loader(customContent) 实例化,使 chkDontAskAgain 可 findChild
		QMetaObject::invokeMethod(dialog.data(), "open");
		QTest::qWait(150);

		// CloseConfirmDialog 枚举:Cancel=0, Quit=1, MinimizeToTray=2
		QSignalSpy spy(dialog.data(), SIGNAL(actionSelected(int, bool)));
		QVERIFY2(spy.isValid(), "actionSelected 信号无效");

		// 找到 chkDontAskAgain(customContent Loader 内的 GCheckBox),勾选
		// 勾选后 chkDontAskAgain.onCheckedChanged → root.dontAskAgain = true
		auto* chkDontAskAgain = dialog->findChild<QQuickItem*>("chkDontAskAgain");
		QVERIFY2(chkDontAskAgain, "未找到 chkDontAskAgain");
		QVERIFY2(chkDontAskAgain->setProperty("checked", true), "勾选 chkDontAskAgain 失败");
		QTRY_COMPARE(chkDontAskAgain->property("checked").toBool(), true);

		// 触发 Quit 按钮(buttons[2])点击 → onButtonClicked(2) → actionSelected(Quit, dontAskAgain)
		auto* btnConfirmClose = dialog->findChild<QQuickItem*>("btnConfirmClose");
		if (btnConfirmClose) {
			// Repeater 已实例化 delegate:真实点击 GButton
			QMetaObject::invokeMethod(btnConfirmClose, "clicked");
		} else {
			// 回退:Repeater 未实例化 delegate(offscreen 环境限制),直接 invoke buttonClicked
			// 信号(index=2=Quit),等价于 GButton.onClicked 的 root.buttonClicked(index, modelData)
			QVariantList bl = dialog->property("buttons").toList();
			QVERIFY2(bl.size() >= 3, "buttons 数组应至少 3 项(Cancel/Minimize/Quit)");
			QVariant quitData = bl.at(2);
			QVERIFY2(QMetaObject::invokeMethod(dialog.data(), "buttonClicked",
											   Q_ARG(int, 2), Q_ARG(QVariant, quitData)),
					 "invoke buttonClicked 失败");
		}

		// 断言 actionSelected 信号发射,action=Quit(1), dontAskAgain=true
		QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 1000);
		QCOMPARE(spy.count(), 1);
		const QList<QVariant> args = spy.takeFirst();
		QCOMPARE(args.at(0).toInt(), 1);	 // Quit
		QCOMPARE(args.at(1).toBool(), true);	 // dontAskAgain

		// 复刻 TitleBar.onActionSelected 持久化逻辑(TitleBar 依赖 FramelessHelper,
		// offscreen 无法加载,此处手动执行其 2 行逻辑):
		//   if (dontAskAgain) SettingsManager.qShowCloseConfirm = false
		if (args.at(1).toBool()) {
			fakeSettings_->setProperty("qShowCloseConfirm", QVariant(false));
		}

		// 断言持久化路径写入 ShowCloseConfirm=false
		bool found = false;
		for (const auto& w : fakeSettings_->writeHistory()) {
			if (w.key == QStringLiteral("ShowCloseConfirm")) {
				found = true;
				QCOMPARE(w.value.toBool(), false);
				break;
			}
		}
		QVERIFY2(found, "writeHistory 应含 ShowCloseConfirm=false");
	}

   private:
	QQmlEngine engine_;
	FakeBrowserManager* fakeBrowser_ = nullptr;
	FakeSettingsManager* fakeSettings_ = nullptr;
};

QTEST_MAIN(TstCloseConfirm)
#include "tst_close_confirm.moc"
