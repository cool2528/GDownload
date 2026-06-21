#pragma once

#include "browser_manager.h"  // BrowserManagerImpl

#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace gdl {
namespace tests {

// 单次 RPC 调用记录:方法名 + 参数列表
struct FakeRpcCall {
	QString method;
	QVariantList args;
};

// 测试期 BrowserManager 替身
//
// 硬约束:必须继承 BrowserManagerImpl(非 IBrowserManager 接口)。
// 原因:mainwindow.cxx 用 qmlRegisterSingletonInstance<BrowserManagerImpl>
//   + static_cast<BrowserManagerImpl*>(IBrowserManager*) 注册单例。
//   若 Fake 继承 IBrowserManager,该 static_cast 为未定义行为(对象实际类型非
//   BrowserManagerImpl 子类)。继承 Impl 后,FakeBrowserManager* 可安全
//   static_cast 为 BrowserManagerImpl*,虚表与元对象链完整。
//
// 不加 Q_OBJECT:避免 moc 生成引用 BrowserManagerImpl::staticMetaObject 等
//   Impl 符号的代码 —— 这些符号仅存在于 gdownload.exe,qml_ui_support 无法链接。
//   Fake 继承 Impl 的元对象,QML 侧元对象访问不受影响。
//
// QML 调用如何命中 Fake override:
//   Fake 无独立 moc 元对象,QML 调用 BrowserManager.AddHttpTask(...) 经
//   BrowserManagerImpl::staticMetaObject 派发到 Impl::qt_metacall,后者对
//   Q_INVOKABLE 槽以 `this->AddHttpTask(...)` 形式调用。AddHttpTask 在
//   IBrowserManager 中为纯虚,Impl 内为 override,故该调用经虚表派发到
//   最派生类(Fake)的 override 实现。因此 Fake 即使无 Q_OBJECT,QML 侧
//   调用与 C++ 直接调用都会命中 Fake 的 override。
//
// Phase 4(Task 9):override 关键 RPC 方法,记录调用历史到 history_,
//   不真正调用 aria2c。其余方法继承 Impl 行为(测试期不触发)。
class FakeBrowserManager : public gdl::ui::browser::BrowserManagerImpl {
   public:
	explicit FakeBrowserManager(QObject* parent = nullptr)
		: gdl::ui::browser::BrowserManagerImpl(parent) {}

	// ===== 任务添加 =====
	// 记录 URL/参数,返回 true 模拟添加成功,不调用 aria2c
	bool AddHttpTask(const QVariantList& urls, const QVariantMap& options) override {
		QVariantList args;
		args.append(QVariant(urls));
		args.append(QVariant(options));
		record(QStringLiteral("AddHttpTask"), args);
		return true;
	}

	bool AddTorrentTask(const QString& torrent, const QVariantMap& options) override {
		QVariantList args;
		args.append(QVariant(torrent));
		args.append(QVariant(options));
		record(QStringLiteral("AddTorrentTask"), args);
		return true;
	}

	bool AddMetalinkTask(const QString& metalink, const QVariantMap& options) override {
		QVariantList args;
		args.append(QVariant(metalink));
		args.append(QVariant(options));
		record(QStringLiteral("AddMetalinkTask"), args);
		return true;
	}

	// ===== 任务暂停/恢复 =====
	bool PauseTask(int page_index, const QString& gid) override {
		QVariantList args;
		args.append(QVariant(page_index));
		args.append(QVariant(gid));
		record(QStringLiteral("PauseTask"), args);
		return true;
	}

	bool ForcePauseTask(int page_index, const QString& gid) override {
		QVariantList args;
		args.append(QVariant(page_index));
		args.append(QVariant(gid));
		record(QStringLiteral("ForcePauseTask"), args);
		return true;
	}

	bool UnpauseTask(int page_index, const QString& gid) override {
		QVariantList args;
		args.append(QVariant(page_index));
		args.append(QVariant(gid));
		record(QStringLiteral("UnpauseTask"), args);
		return true;
	}

	bool PauseAllTask(int page_index) override {
		QVariantList args;
		args.append(QVariant(page_index));
		record(QStringLiteral("PauseAllTask"), args);
		return true;
	}

	bool UnpauseAllTask(int page_index) override {
		QVariantList args;
		args.append(QVariant(page_index));
		record(QStringLiteral("UnpauseAllTask"), args);
		return true;
	}

	// ===== 任务删除 =====
	bool RemoveTask(int page_index, const QString& gid, bool is_remove_file = false) override {
		QVariantList args;
		args.append(QVariant(page_index));
		args.append(QVariant(gid));
		args.append(QVariant(is_remove_file));
		record(QStringLiteral("RemoveTask"), args);
		return true;
	}

	bool RemoveAllTask(int page_index, bool is_remove_file = true) override {
		QVariantList args;
		args.append(QVariant(page_index));
		args.append(QVariant(is_remove_file));
		record(QStringLiteral("RemoveAllTask"), args);
		return true;
	}

	// ===== 测试访问器(非 Q_INVOKABLE,仅 C++ 集成测试调用) =====

	// 全部调用历史
	const QList<FakeRpcCall>& rpcCallHistory() const { return history_; }

	// 最近一次调用;历史为空时返回空 record
	FakeRpcCall lastRpcCall() const {
		return history_.isEmpty() ? FakeRpcCall{} : history_.last();
	}

	// 最近一次调用的方法名;历史为空时返回空串
	QString lastRpcMethod() const {
		return history_.isEmpty() ? QString() : history_.last().method;
	}

	// 调用次数
	int rpcCallCount() const { return history_.size(); }

	// 清空历史(测试 init 调用)
	void clearHistory() { history_.clear(); }

   private:
	// 记录一次调用
	void record(const QString& method, const QVariantList& args) {
		history_.append({method, args});
	}

	QList<FakeRpcCall> history_;
};

}  // namespace tests
}  // namespace gdl
