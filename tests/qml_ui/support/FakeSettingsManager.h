#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QtQml/qqml.h>

namespace gdl {
namespace tests {

// 单次 setter 调用记录:key + value
struct FakeSettingsWrite {
	QString key;
	QVariant value;
};

// 测试期 Settings 替身(独立 QObject 版)
//
// 设计变更说明(原 Task 9 版本继承 SettingsImpl,现改为独立 QObject):
// 原设计 Fake 继承 SettingsImpl,期望经 Impl::qt_metacall 虚派发命中 Fake override。
// 但 SettingsImpl 的 staticMetaObject / vtable / 构造析构 / 全部 *Changed 信号 / Save() /
// SetValue 模板实例等符号仅存在于 gdownload.exe(可执行文件,非可链接库),集成测试 exe
// 无法链接(LNK2019: 86 个未解析外部符号)。将主项目重构为可链接库超出 Phase 4 范围,
// 故改为独立 QObject:自带 Q_OBJECT 元对象,经 FAKE_SETTING 宏复刻 SETTING_PROPERTY
// 生成的 Q_PROPERTY(q##NAME / Get##NAME / Set##NAME / NAME##Changed)接口,不依赖任何
// Impl 符号。QML 端 qXxx 读写与 SetXxx / SetAria2Xxx 调用均直接命中本类实现,
// 写入内存 store_ + 记录 write_history_ + emit *Changed 信号,不落盘。
//
// FAKE_SETTING 宏结构镜像 src/App/ui/Settings/setting.h 的 SETTING_PROPERTY
// (private Q_PROPERTY + public Q_INVOKABLE Get/Set + Q_SIGNAL Changed),moc 可正确处理。
//
// 默认值:initDefaults() 设置集成测试触碰的 4 个 ConnectionPerformance 字段
// (MaxConcurrentDownloads=5 / MaxConnectionPerServer=16 / Split=64 / MinSplitSize=20)
// 与 ShowCloseConfirm/CloseToTray/Dir 等,使页面 SpinBox 不因 0 值被 clamp 导致
// hasChanges 误真。clearHistory() 仅清 write_history_,保留 store_ 默认值。
class FakeSettingsManager : public QObject {
	Q_OBJECT

   public:
	explicit FakeSettingsManager(QObject* parent = nullptr) : QObject(parent) { initDefaults(); }

	// 复刻 SettingsImpl 的 54 个 SETTING_PROPERTY:Q_PROPERTY q##NAME + Get/Set + Changed 信号
	// Set##NAME 写入 store_ + 记录 write_history_ + emit Changed;Get##NAME 读 store_(miss 返回默认)
#define FAKE_SETTING(TYPE, NAME) \
   private: \
	Q_PROPERTY(TYPE q##NAME READ Get##NAME WRITE Set##NAME NOTIFY NAME##Changed) \
   public: \
	Q_INVOKABLE TYPE Get##NAME() const { return store_.value(QStringLiteral(#NAME)).value<TYPE>(); } \
	Q_INVOKABLE void Set##NAME(TYPE value) { writeSetting(QStringLiteral(#NAME), QVariant::fromValue(value)); Q_EMIT NAME##Changed(); } \
	Q_SIGNAL void NAME##Changed();

	FAKE_SETTING(QSize, WindowSize)
	FAKE_SETTING(QString, Theme)
	FAKE_SETTING(QString, Language)
	FAKE_SETTING(QString, BtExludeTracker)
	FAKE_SETTING(QString, BtTracker)
	FAKE_SETTING(QString, Dir)
	FAKE_SETTING(int, ListenPort)
	FAKE_SETTING(int, RpcListenPort)
	FAKE_SETTING(QString, RpcSecret)
	FAKE_SETTING(int, Split)
	FAKE_SETTING(QString, UserAgent)
	FAKE_SETTING(QString, AllProxy)
	FAKE_SETTING(int, DhtListenPort)
	FAKE_SETTING(int, MaxConcurrentDownloads)
	FAKE_SETTING(QString, ConfPath)
	FAKE_SETTING(QString, TrackerSourceUrls)
	FAKE_SETTING(QString, SaveSession)
	FAKE_SETTING(bool, IsSaveSession)
	FAKE_SETTING(bool, EnableGlobalProxy)
	FAKE_SETTING(QString, GlobalProxy)
	FAKE_SETTING(bool, ListenClipboard)
	FAKE_SETTING(bool, AutoResumeTask)
	FAKE_SETTING(bool, AutoStart)
	FAKE_SETTING(bool, RememberWindowPosition)
	FAKE_SETTING(bool, EnableTrayIcon)
	FAKE_SETTING(bool, EnableNotification)
	FAKE_SETTING(bool, EnableAutoShutdown)
	FAKE_SETTING(bool, EnableAutoUpdate)
	FAKE_SETTING(bool, EnableGithubAccelerate)
	FAKE_SETTING(QPoint, WindowPosition)
	FAKE_SETTING(QString, BaiduPanCookies)
	FAKE_SETTING(QString, TrackerSourceNames)
	FAKE_SETTING(bool, EnableTrackerSourceAutoUpdate)
	FAKE_SETTING(bool, ShowCloseConfirm)
	FAKE_SETTING(bool, CloseToTray)
	FAKE_SETTING(int, MaxDownloadLimit)
	FAKE_SETTING(int, MaxOverallDownloadLimit)
	FAKE_SETTING(int, MaxUploadLimit)
	FAKE_SETTING(int, MaxOverallUploadLimit)
	FAKE_SETTING(int, LowestSpeedLimit)
	FAKE_SETTING(int, MaxConnectionPerServer)
	FAKE_SETTING(int, MinSplitSize)
	FAKE_SETTING(int, OnCompleteAction)
	FAKE_SETTING(QString, CustomCompleteCommand)
	FAKE_SETTING(int, OnErrorAction)
	FAKE_SETTING(QString, CustomErrorCommand)
	FAKE_SETTING(int, OnStartAction)
	FAKE_SETTING(int, Timeout)
	FAKE_SETTING(int, ConnectTimeout)
	FAKE_SETTING(int, MaxTries)
	FAKE_SETTING(int, RetryWait)
	FAKE_SETTING(bool, EnableDht)
	FAKE_SETTING(int, BtMaxPeers)
	FAKE_SETTING(bool, BtRequireCrypto)
	FAKE_SETTING(QString, Ed2kNickname)
	FAKE_SETTING(int, Ed2kTcpPort)
	FAKE_SETTING(int, Ed2kUdpPort)
	FAKE_SETTING(bool, Ed2kEnableKad)
	FAKE_SETTING(bool, Ed2kEnableObfuscation)
	FAKE_SETTING(bool, Ed2kAutoConnect)
	FAKE_SETTING(int, Ed2kMaxConcurrentTasks)
	FAKE_SETTING(QString, Ed2kSharedDirs)
	FAKE_SETTING(QString, Ed2kServerMetUrl)
	FAKE_SETTING(QString, Ed2kNodesDatUrl)
	FAKE_SETTING(bool, Ed2kAutoSyncSources)
#undef FAKE_SETTING

	// ===== SettingsImpl 显式声明的 Q_INVOKABLE 别名(SetAria2Xxx)=====
	// 委托给对应 Set##NAME,记录同一 key + emit 同一 Changed 信号
	Q_INVOKABLE QString GenerateRpcSecret() const {
		return QStringLiteral("test-fake-secret-0123456789abcdef");
	}
	Q_INVOKABLE QString GetDefaultBrowserUserAgent() const {
		return QStringLiteral(
			"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
			"Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");
	}
	Q_INVOKABLE void SetAria2Dir(const QString& dir) { SetDir(dir); }
	Q_INVOKABLE void SetAria2GlobalProxy(const QString& proxy) { SetGlobalProxy(proxy); }
	Q_INVOKABLE void SetAria2AutoResumeTask(bool enable) { SetAutoResumeTask(enable); }
	Q_INVOKABLE void SetAria2MaxDownloadLimit(int value) { SetMaxDownloadLimit(value); }
	Q_INVOKABLE void SetAria2MaxOverallDownloadLimit(int value) { SetMaxOverallDownloadLimit(value); }
	Q_INVOKABLE void SetAria2MaxUploadLimit(int value) { SetMaxUploadLimit(value); }
	Q_INVOKABLE void SetAria2MaxOverallUploadLimit(int value) { SetMaxOverallUploadLimit(value); }
	Q_INVOKABLE void SetAria2LowestSpeedLimit(int value) { SetLowestSpeedLimit(value); }
	Q_INVOKABLE void SetAria2MaxConcurrentDownloads(int value) { SetMaxConcurrentDownloads(value); }
	Q_INVOKABLE void SetAria2Split(int value) { SetSplit(value); }
	Q_INVOKABLE void SetAria2MaxConnectionPerServer(int value) { SetMaxConnectionPerServer(value); }
	Q_INVOKABLE void SetAria2MinSplitSize(int sizeMB) { SetMinSplitSize(sizeMB); }
	Q_INVOKABLE void SetAria2Timeout(int value) { SetTimeout(value); }
	Q_INVOKABLE void SetAria2ConnectTimeout(int value) { SetConnectTimeout(value); }
	Q_INVOKABLE void SetAria2MaxTries(int value) { SetMaxTries(value); }
	Q_INVOKABLE void SetAria2RetryWait(int value) { SetRetryWait(value); }
	Q_INVOKABLE void SetAria2EnableDht(bool enable) { SetEnableDht(enable); }
	Q_INVOKABLE void SetAria2BtMaxPeers(int value) { SetBtMaxPeers(value); }
	Q_INVOKABLE void SetAria2BtRequireCrypto(bool enable) { SetBtRequireCrypto(enable); }
	Q_INVOKABLE void SetAria2UserAgent(const QString& userAgent) { SetUserAgent(userAgent); }

	// ===== 测试访问器(非 Q_INVOKABLE,仅 C++ 集成测试调用)=====

	// 全部写入历史
	const QList<FakeSettingsWrite>& writeHistory() const { return write_history_; }

	// 最近一次写入;历史为空时返回空 record
	FakeSettingsWrite lastWrite() const {
		return write_history_.isEmpty() ? FakeSettingsWrite{} : write_history_.last();
	}

	// 最近一次写入的 key;历史为空时返回空串
	QString lastWrittenKey() const {
		return write_history_.isEmpty() ? QString() : write_history_.last().key;
	}

	// 最近一次写入的 value;历史为空时返回无效 QVariant
	QVariant lastWrittenValue() const {
		return write_history_.isEmpty() ? QVariant() : write_history_.last().value;
	}

	// 写入次数
	int writeCount() const { return write_history_.size(); }

	// 清空写入历史(测试 init 调用)。仅清 write_history_,保留 store_ 默认值,
	// 使页面加载时 GetXxx 返回稳定默认值,避免 SpinBox clamp 导致 hasChanges 误真
	void clearHistory() { write_history_.clear(); }

   private:
	// 记录一次 setter 调用并写入内存 store_
	void writeSetting(const QString& key, const QVariant& value) {
		write_history_.append({key, value});
		store_[key] = value;
	}

	// 初始化集成测试触碰字段的默认值(对齐 src/App/ui/Settings/setting.h 的 Default())
	void initDefaults() {
		store_[QStringLiteral("MaxConcurrentDownloads")] = 5;
		store_[QStringLiteral("MaxConnectionPerServer")] = 16;
		store_[QStringLiteral("Split")] = 64;
		store_[QStringLiteral("MinSplitSize")] = 20;
		store_[QStringLiteral("UserAgent")] = GetDefaultBrowserUserAgent();
		store_[QStringLiteral("ShowCloseConfirm")] = true;
		store_[QStringLiteral("CloseToTray")] = false;
		store_[QStringLiteral("Dir")] = QStringLiteral("C:/Downloads");
		store_[QStringLiteral("Theme")] = QStringLiteral("Light");
		store_[QStringLiteral("Language")] = QStringLiteral("en_US");
		store_[QStringLiteral("ListenClipboard")] = true;
		store_[QStringLiteral("EnableAutoUpdate")] = true;
		store_[QStringLiteral("AutoResumeTask")] = true;
		store_[QStringLiteral("RememberWindowPosition")] = true;
		store_[QStringLiteral("EnableTrayIcon")] = true;
		store_[QStringLiteral("Ed2kNickname")] = QStringLiteral("GDownload");
		store_[QStringLiteral("Ed2kTcpPort")] = 4662;
		store_[QStringLiteral("Ed2kUdpPort")] = 4672;
		store_[QStringLiteral("Ed2kEnableKad")] = true;
		store_[QStringLiteral("Ed2kEnableObfuscation")] = false;
		store_[QStringLiteral("Ed2kAutoConnect")] = true;
		store_[QStringLiteral("Ed2kMaxConcurrentTasks")] = 5;
		store_[QStringLiteral("Ed2kSharedDirs")] = QStringLiteral("");
		store_[QStringLiteral("Ed2kServerMetUrl")] = QStringLiteral("http://upd.emule-security.org/server.met");
		store_[QStringLiteral("Ed2kNodesDatUrl")] = QStringLiteral("http://upd.emule-security.org/nodes.dat");
		store_[QStringLiteral("Ed2kAutoSyncSources")] = true;
	}

	QList<FakeSettingsWrite> write_history_;
	QHash<QString, QVariant> store_;
};

}  // namespace tests
}  // namespace gdl
