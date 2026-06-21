#include "FakeSettingsManager.h"

// FakeSettingsManager:内存版 Settings 替身
//
// 10 个高频 setter/getter(WindowSize / Theme / Language / Dir /
// ShowCloseConfirm / CloseToTray / EnableTrayIcon / MaxConcurrentDownloads /
// ListenClipboard / WindowPosition)以 inline 形式定义于头文件,
// 写入内存 store_ + 记录 writeHistory_ + emit *Changed,不落盘。
// 其余 44 个属性沿用 SettingsImpl 的 ini 读写。
//
// 不加 Q_OBJECT:同 FakeBrowserManager,继承 Impl 元对象避免链接 Impl 符号。
// QML 写 qXxx 属性经 Impl::qt_metacall 虚派发命中 Fake override。
//
// AUTOMOC 扫描本 .cxx 时不发现 Q_OBJECT,跳过 moc 生成,符合预期。
