#pragma once

#include "settings_manager.h"  // SettingsImpl

#include <QHash>
#include <QList>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>

namespace gdl {
namespace tests {

// 单次 setter 调用记录:key + value
struct FakeSettingsWrite {
	QString key;
	QVariant value;
};

// 测试期 Settings 替身
//
// 硬约束:必须继承 SettingsImpl(非 ISettings 接口)。
// 原因:同 FakeBrowserManager —— mainwindow.cxx 用
//   qmlRegisterSingletonInstance<SettingsImpl> + static_cast<SettingsImpl*>
//   注册单例。继承接口会导致 static_cast 未定义行为。
//
// 不加 Q_OBJECT:避免 moc 生成引用 SettingsImpl::staticMetaObject / 各 *Changed
//   信号等 Impl 符号的代码 —— 这些符号仅存在于 gdownload.exe,qml_ui_support
//   无法链接。Fake 继承 Impl 的元对象,QML 侧元对象访问不受影响。
//
// QML 调用如何命中 Fake override:
//   SETTING_PROPERTY 宏在 Impl 内生成 `void Set##NAME(TYPE value) override`
//   并标记 Q_INVOKABLE。Fake override 同名同签名的虚函数后,QML 经
//   Impl::qt_metacall 调用 `this->Set##NAME(value)` 时经虚表派发到 Fake。
//   同理 getter `Get##NAME() const` 也可被 override 拦截。因此 QML 写
//   qXxx 属性与 C++ 直接调 SetXxx 都会命中 Fake 的内存实现,不落盘。
//
// Phase 4(Task 9):override 10 个集成测试高频触碰的 setter/getter,
//   写入内存 store_ + 记录 writeHistory_ + emit *Changed 信号,
//   不调用 SettingsImpl::SetValue(避免 ini 文件 IO)。
//   未 override 的 44 个属性沿用 Impl 的 ini 读写行为。
class FakeSettingsManager : public gdl::ui::settings::SettingsImpl {
   public:
	explicit FakeSettingsManager(QObject* parent = nullptr)
		: gdl::ui::settings::SettingsImpl(parent) {}

	// ===== 窗口与界面 =====

	void SetWindowSize(QSize value) override {
		record(QStringLiteral("WindowSize"), QVariant(value));
		store_[QStringLiteral("WindowSize")] = QVariant(value);
		Q_EMIT WindowSizeChanged();
	}
	QSize GetWindowSize() const override {
		auto it = store_.constFind(QStringLiteral("WindowSize"));
		if (it != store_.constEnd()) return it->toSize();
		return gdl::ui::settings::SettingsImpl::GetWindowSize();
	}

	void SetWindowPosition(QPoint value) override {
		record(QStringLiteral("WindowPosition"), QVariant(value));
		store_[QStringLiteral("WindowPosition")] = QVariant(value);
		Q_EMIT WindowPositionChanged();
	}
	QPoint GetWindowPosition() const override {
		auto it = store_.constFind(QStringLiteral("WindowPosition"));
		if (it != store_.constEnd()) return it->toPoint();
		return gdl::ui::settings::SettingsImpl::GetWindowPosition();
	}

	void SetTheme(QString value) override {
		record(QStringLiteral("Theme"), QVariant(value));
		store_[QStringLiteral("Theme")] = QVariant(value);
		Q_EMIT ThemeChanged();
	}
	QString GetTheme() const override {
		auto it = store_.constFind(QStringLiteral("Theme"));
		if (it != store_.constEnd()) return it->toString();
		return gdl::ui::settings::SettingsImpl::GetTheme();
	}

	void SetLanguage(QString value) override {
		record(QStringLiteral("Language"), QVariant(value));
		store_[QStringLiteral("Language")] = QVariant(value);
		Q_EMIT LanguageChanged();
	}
	QString GetLanguage() const override {
		auto it = store_.constFind(QStringLiteral("Language"));
		if (it != store_.constEnd()) return it->toString();
		return gdl::ui::settings::SettingsImpl::GetLanguage();
	}

	// ===== 下载路径 =====

	void SetDir(QString value) override {
		record(QStringLiteral("Dir"), QVariant(value));
		store_[QStringLiteral("Dir")] = QVariant(value);
		Q_EMIT DirChanged();
	}
	QString GetDir() const override {
		auto it = store_.constFind(QStringLiteral("Dir"));
		if (it != store_.constEnd()) return it->toString();
		return gdl::ui::settings::SettingsImpl::GetDir();
	}

	// ===== 关闭确认与托盘 =====

	void SetShowCloseConfirm(bool value) override {
		record(QStringLiteral("ShowCloseConfirm"), QVariant(value));
		store_[QStringLiteral("ShowCloseConfirm")] = QVariant(value);
		Q_EMIT ShowCloseConfirmChanged();
	}
	bool GetShowCloseConfirm() const override {
		auto it = store_.constFind(QStringLiteral("ShowCloseConfirm"));
		if (it != store_.constEnd()) return it->toBool();
		return gdl::ui::settings::SettingsImpl::GetShowCloseConfirm();
	}

	void SetCloseToTray(bool value) override {
		record(QStringLiteral("CloseToTray"), QVariant(value));
		store_[QStringLiteral("CloseToTray")] = QVariant(value);
		Q_EMIT CloseToTrayChanged();
	}
	bool GetCloseToTray() const override {
		auto it = store_.constFind(QStringLiteral("CloseToTray"));
		if (it != store_.constEnd()) return it->toBool();
		return gdl::ui::settings::SettingsImpl::GetCloseToTray();
	}

	void SetEnableTrayIcon(bool value) override {
		record(QStringLiteral("EnableTrayIcon"), QVariant(value));
		store_[QStringLiteral("EnableTrayIcon")] = QVariant(value);
		Q_EMIT EnableTrayIconChanged();
	}
	bool GetEnableTrayIcon() const override {
		auto it = store_.constFind(QStringLiteral("EnableTrayIcon"));
		if (it != store_.constEnd()) return it->toBool();
		return gdl::ui::settings::SettingsImpl::GetEnableTrayIcon();
	}

	// ===== 下载行为 =====

	void SetMaxConcurrentDownloads(int value) override {
		record(QStringLiteral("MaxConcurrentDownloads"), QVariant(value));
		store_[QStringLiteral("MaxConcurrentDownloads")] = QVariant(value);
		Q_EMIT MaxConcurrentDownloadsChanged();
	}
	int GetMaxConcurrentDownloads() const override {
		auto it = store_.constFind(QStringLiteral("MaxConcurrentDownloads"));
		if (it != store_.constEnd()) return it->toInt();
		return gdl::ui::settings::SettingsImpl::GetMaxConcurrentDownloads();
	}

	void SetListenClipboard(bool value) override {
		record(QStringLiteral("ListenClipboard"), QVariant(value));
		store_[QStringLiteral("ListenClipboard")] = QVariant(value);
		Q_EMIT ListenClipboardChanged();
	}
	bool GetListenClipboard() const override {
		auto it = store_.constFind(QStringLiteral("ListenClipboard"));
		if (it != store_.constEnd()) return it->toBool();
		return gdl::ui::settings::SettingsImpl::GetListenClipboard();
	}

	// ===== 测试访问器(非 Q_INVOKABLE,仅 C++ 集成测试调用) =====

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

	// 清空历史与内存存储(测试 init 调用)
	void clearHistory() {
		write_history_.clear();
		store_.clear();
	}

   private:
	// 记录一次 setter 调用
	void record(const QString& key, const QVariant& value) {
		write_history_.append({key, value});
	}

	QList<FakeSettingsWrite> write_history_;
	QHash<QString, QVariant> store_;
};

}  // namespace tests
}  // namespace gdl
