#include "FakeSettingsManager.h"

// Phase 1:空实现
// FakeSettingsManager 当前无 override 方法,所有行为继承自 SettingsImpl。
// 本 .cxx 用于触发 AUTOMOC 对 Q_OBJECT 的 moc 处理。
// Phase 4 会在此追加 override 方法的实现 + history 记录逻辑。
