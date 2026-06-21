#include "TestStubs.h"

// TestStubs 全部实现内联在头文件中,本 .cxx 仅用于触发 AUTOMOC
// 对各桩类的 Q_OBJECT 进行 moc 处理,生成元对象实现。
// 不在此追加业务逻辑;Phase 4(Task 9)重构主项目为库后,本文件可移除,
// 测试改用真实 BrowserManagerImpl / SettingsImpl 等 Impl 符号。
