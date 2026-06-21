#include "FakeBrowserManager.h"

// FakeBrowserManager:独立 QObject 版 RPC 调用记录替身
//
// 所有 Q_INVOKABLE 方法以 inline 形式定义于头文件,记录调用到 history_ 后返回成功,
// 不调用 aria2c。本 .cxx 无需额外实现。
//
// 独立 QObject 设计(替代原 Task 9 继承 BrowserManagerImpl 方案):
// 原方案继承 Impl 期望虚派发命中 override,但 Impl 的 staticMetaObject / vtable /
// 构造析构 / *Changed 信号等符号仅存于 gdownload.exe,测试 exe 无法链接(LNK2019)。
// 改为独立 QObject 自带 Q_OBJECT 元对象,不依赖 Impl 符号,可直接链接 + 实例化 +
// qmlRegisterSingletonInstance<FakeBrowserManager> 注册为 "BrowserManager" 单例。
// QML 调用经 Fake 自身元对象派发,直接命中本类 Q_INVOKABLE。
//
// AUTOMOC 扫描本 .cxx 时发现 #include "FakeBrowserManager.h" 且头含 Q_OBJECT,
// 生成 moc_FakeBrowserManager.cpp 链入 qml_ui_support,提供元对象实现。
