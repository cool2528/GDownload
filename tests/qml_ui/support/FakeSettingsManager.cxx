#include "FakeSettingsManager.h"

// FakeSettingsManager:独立 QObject 版内存 Settings 替身
//
// 54 个 SETTING_PROPERTY 经 FAKE_SETTING 宏复刻为 Q_PROPERTY + Get/Set + Changed,
// 写入内存 store_ + 记录 write_history_ + emit Changed,不落盘。20 个 SetAria2Xxx
// 别名委托给对应 Set##NAME。本 .cxx 无需额外实现。
//
// 独立 QObject 设计(替代原 Task 9 继承 SettingsImpl 方案):
// 原方案继承 Impl 期望虚派发命中 override,但 Impl 的 staticMetaObject / vtable /
// 构造析构 / *Changed 信号 / Save() / SetValue 模板实例等符号仅存于 gdownload.exe,
// 测试 exe 无法链接(LNK2019)。改为独立 QObject 自带 Q_OBJECT 元对象,不依赖 Impl
// 符号。QML 读写 qXxx 与调用 SetXxx/SetAria2Xxx 均直接命中本类实现。
//
// AUTOMOC 扫描本 .cxx 时发现 #include "FakeSettingsManager.h" 且头含 Q_OBJECT,
// 生成 moc_FakeSettingsManager.cpp 链入 qml_ui_support,提供元对象实现。
