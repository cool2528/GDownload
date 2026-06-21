#include "FakeBrowserManager.h"

// FakeBrowserManager:RPC 调用记录替身
//
// 所有 override 方法(AddHttpTask / PauseTask 等)以 inline 形式定义于头文件,
// 记录调用到 history_ 后返回成功,不调用 aria2c。本 .cxx 无需额外实现。
//
// 不加 Q_OBJECT:Fake 继承 BrowserManagerImpl 的元对象,moc 不为本类生成
// 独立元对象代码(避免引用仅存于 gdownload.exe 的 Impl 符号)。
// QML 调用经 Impl::qt_metacall 虚派发到 Fake override,C++ 直接调用亦然。
//
// AUTOMOC 扫描本 .cxx 时不发现 Q_OBJECT,跳过 moc 生成,符合预期。
