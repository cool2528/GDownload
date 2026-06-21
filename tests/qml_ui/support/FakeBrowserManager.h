#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

namespace gdl {
namespace tests {

// 单次 RPC 调用记录:方法名 + 参数列表
struct FakeRpcCall {
	QString method;
	QVariantList args;
};

// 测试期 BrowserManager 替身(独立 QObject 版)
//
// 设计变更说明(原 Task 9 版本继承 BrowserManagerImpl,现改为独立 QObject):
// 原设计 Fake 继承 BrowserManagerImpl,期望经 Impl::qt_metacall 虚派发命中 Fake override。
// 但 BrowserManagerImpl 的 staticMetaObject / vtable / 构造析构 / 全部 *Changed 信号等
// 符号仅存在于 gdownload.exe(可执行文件,非可链接库),集成测试 exe 无法链接这些符号
// (LNK2019: 86 个未解析外部符号)。将主项目重构为可链接库超出 Phase 4 范围,
// 故改为独立 QObject:自带 Q_OBJECT 元对象,不依赖任何 Impl 符号,可被测试 exe 直接
// 链接 + 实例化 + 经 qmlRegisterSingletonInstance<FakeBrowserManager> 注册为
// "BrowserManager" 单例。QML 端调用经 Fake 自身元对象派发,直接命中本类 Q_INVOKABLE。
//
// 与视觉用例 TestStubs.h::TestBrowserManager 的区别:本类记录调用历史(history_),
// 供集成测试断言;TestBrowserManager 为空实现桩,仅供视觉渲染。
//
// 接口对齐 BrowserManagerImpl 的 QML 面:覆盖 QML 页面实际引用的全部 Q_INVOKABLE 方法
// (AddHttpTask / AddTorrentTask / AddMetalinkTask / PauseTask / ... / GetActiveDownloadModel
// 等模型方法返回 nullptr,使页面以空状态渲染)。信号声明与生产一致(供 QML Connections
// 解析,测试期不发射)。
class FakeBrowserManager : public QObject {
	Q_OBJECT

   public:
	explicit FakeBrowserManager(QObject* parent = nullptr) : QObject(parent) {}

	// ===== 任务添加(记录到 history_)=====
	Q_INVOKABLE bool AddHttpTask(const QVariantList& urls, const QVariantMap& options) {
		record(QStringLiteral("AddHttpTask"), {QVariant(urls), QVariant(options)});
		return true;
	}
	Q_INVOKABLE bool AddTorrentTask(const QString& torrent, const QVariantMap& options) {
		record(QStringLiteral("AddTorrentTask"), {QVariant(torrent), QVariant(options)});
		return true;
	}
	Q_INVOKABLE bool AddMetalinkTask(const QString& metalink, const QVariantMap& options) {
		record(QStringLiteral("AddMetalinkTask"), {QVariant(metalink), QVariant(options)});
		return true;
	}

	// ===== 任务暂停/恢复(记录)=====
	Q_INVOKABLE bool PauseTask(int page_index, const QString& gid) {
		record(QStringLiteral("PauseTask"), {page_index, QVariant(gid)});
		return true;
	}
	Q_INVOKABLE bool ForcePauseTask(int page_index, const QString& gid) {
		record(QStringLiteral("ForcePauseTask"), {page_index, QVariant(gid)});
		return true;
	}
	Q_INVOKABLE bool UnpauseTask(int page_index, const QString& gid) {
		record(QStringLiteral("UnpauseTask"), {page_index, QVariant(gid)});
		return true;
	}
	Q_INVOKABLE bool PauseAllTask(int page_index) {
		record(QStringLiteral("PauseAllTask"), {page_index});
		return true;
	}
	Q_INVOKABLE bool UnpauseAllTask(int page_index) {
		record(QStringLiteral("UnpauseAllTask"), {page_index});
		return true;
	}
	Q_INVOKABLE bool ForcePauseAllTask() {
		record(QStringLiteral("ForcePauseAllTask"), {});
		return true;
	}

	// ===== 任务删除(记录)=====
	Q_INVOKABLE bool RemoveTask(int page_index, const QString& gid, bool is_remove_file = false) {
		record(QStringLiteral("RemoveTask"), {page_index, QVariant(gid), is_remove_file});
		return true;
	}
	Q_INVOKABLE bool RemoveAllTask(int page_index, bool is_remove_file = true) {
		record(QStringLiteral("RemoveAllTask"), {page_index, is_remove_file});
		return true;
	}
	Q_INVOKABLE bool ForceRemoveTask(const QString& gid) {
		record(QStringLiteral("ForceRemoveTask"), {QVariant(gid)});
		return true;
	}
	Q_INVOKABLE bool RemoveDownloadResult(const QString& gid) {
		record(QStringLiteral("RemoveDownloadResult"), {QVariant(gid)});
		return true;
	}
	Q_INVOKABLE bool PurgeDownloadResult() {
		record(QStringLiteral("PurgeDownloadResult"), {});
		return true;
	}

	// ===== 选项变更(记录)=====
	Q_INVOKABLE bool ChangeOption(const QString& gid, const QVariantMap& options) {
		record(QStringLiteral("ChangeOption"), {QVariant(gid), QVariant(options)});
		return true;
	}
	Q_INVOKABLE bool ChangeGlobalOption(const QVariantMap& options) {
		record(QStringLiteral("ChangeGlobalOption"), {QVariant(options)});
		return true;
	}

	// ===== 非记录类(QML 兼容,空实现)=====
	Q_INVOKABLE void SyncTrackersServerlist() {}
	Q_INVOKABLE QObject* GetActiveDownloadModel() { return nullptr; }
	Q_INVOKABLE QObject* GetStopedDownloadModel() { return nullptr; }
	Q_INVOKABLE QObject* GetWaitingDownloadModel() { return nullptr; }
	Q_INVOKABLE void OpenFileLocation(const QString& file_path) { Q_UNUSED(file_path); }
	Q_INVOKABLE bool RemoveStopTask(const QString& gid, bool is_remove_file = true) const {
		Q_UNUSED(gid);
		Q_UNUSED(is_remove_file);
		return true;
	}
	Q_INVOKABLE bool RemoveStopTask(int index, bool is_remove_file = true) const {
		Q_UNUSED(index);
		Q_UNUSED(is_remove_file);
		return true;
	}
	Q_INVOKABLE bool RemoveAllStopTask(bool is_remove_file = true) const {
		Q_UNUSED(is_remove_file);
		return true;
	}
	Q_INVOKABLE void RefreshTaskList(int page_index) { Q_UNUSED(page_index); }
	Q_INVOKABLE QObject* GetFilePreviewModel(const QString& file_path) {
		Q_UNUSED(file_path);
		return nullptr;
	}

	// ===== 信号(与生产签名兼容,QML Connections 解析用;测试期不发射)=====
	// sigUpdateTasksMessage 用 QVariant 替代 DownloadTaskInfo(避免引入 download_task_model.h
	// 传递依赖),QML 信号处理器按名连接,参数类型不强制校验
   Q_SIGNALS:
	void sigErrorMessage(const QString& error);
	void sigUpdateTasksMessage(const QVariant& info);
	void sigUpdateActiveProgress(qreal progress);
	void sigUpdateSyncServerList(const QString& list);
	void sigTrackerUpdateStatus(const QString& status);

   public:
	// ===== 测试访问器(非 Q_INVOKABLE,仅 C++ 集成测试调用)=====

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

	// 清空历史(测试 init 调用)。仅清调用历史,不影响单例状态
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
