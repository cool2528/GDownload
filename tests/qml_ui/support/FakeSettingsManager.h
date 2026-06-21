#pragma once

#include "settings_manager.h"  // SettingsImpl

namespace gdl {
namespace tests {

// 测试期 Settings 替身
//
// 硬约束:必须继承 SettingsImpl(非 ISettings 接口)。
// 原因:同 FakeBrowserManager —— mainwindow.cxx 用
//   qmlRegisterSingletonInstance<SettingsImpl> + static_cast<SettingsImpl*>
//   注册单例。继承接口会导致 static_cast 未定义行为。
//
// Phase 1:直接继承,所有 SETTING_PROPERTY(Q_PROPERTY) 自动生效,
//   测试期默认读写 Impl 的 ini 配置。本阶段不实例化 Fake,仅保证头文件可编译。
//   不加 Q_OBJECT:避免 moc 生成引用 SettingsImpl::staticMetaObject / 各 *Changed
//   信号等 Impl 符号的代码 —— 这些符号仅存在于 gdownload.exe,qml_ui_support
//   无法链接。Fake 继承 Impl 的元对象,QML 侧元对象访问不受影响。
// Phase 4(Task 9):加 Q_OBJECT + override / history 方法,届时需把
//   settings_manager.cxx 编入测试库或重构主项目为库以提供 Impl 符号。
class FakeSettingsManager : public gdl::ui::settings::SettingsImpl {
   public:
    explicit FakeSettingsManager(QObject* parent = nullptr)
        : gdl::ui::settings::SettingsImpl(parent) {}

    // Phase 4 在此追加 override / history 方法,例如:
    //   QStringList writtenKeys() const { return written_keys_; }
};

}  // namespace tests
}  // namespace gdl
