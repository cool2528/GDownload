#pragma once

#include "browser_manager.h"  // BrowserManagerImpl

namespace gdl {
namespace tests {

// 测试期 BrowserManager 替身
//
// 硬约束:必须继承 BrowserManagerImpl(非 IBrowserManager 接口)。
// 原因:mainwindow.cxx 用 qmlRegisterSingletonInstance<BrowserManagerImpl>
//   + static_cast<BrowserManagerImpl*>(IBrowserManager*) 注册单例。
//   若 Fake 继承 IBrowserManager,该 static_cast 为未定义行为(对象实际类型非
//   BrowserManagerImpl 子类)。继承 Impl 后,FakeBrowserManager* 可安全
//   static_cast 为 BrowserManagerImpl*,虚表与元对象链完整。
//
// Phase 1:直接继承,所有 BrowserManagerImpl 公开方法(AddHttpTask 等)自动生效,
//   测试期默认调用 Impl 行为。本阶段不实例化 Fake,仅保证头文件可编译。
//   不加 Q_OBJECT:避免 moc 生成引用 BrowserManagerImpl::staticMetaObject 等
//   Impl 符号的代码 —— 这些符号仅存在于 gdownload.exe,qml_ui_support 无法链接。
//   Fake 继承 Impl 的元对象,QML 侧元对象访问不受影响。
// Phase 4(Task 9):加 Q_OBJECT + override 关键方法(AddHttpTask 等),届时需把
//   browser_manager.cxx 编入测试库或重构主项目为库以提供 Impl 符号。
class FakeBrowserManager : public gdl::ui::browser::BrowserManagerImpl {
   public:
    explicit FakeBrowserManager(QObject* parent = nullptr)
        : gdl::ui::browser::BrowserManagerImpl(parent) {}

    // Phase 4 在此追加 override 方法,例如:
    //   bool AddHttpTask(const QVariantList& urls, const QVariantMap& options) override;
    //   QList<RpcCall> rpcCallHistory() const { return history_; }
};

}  // namespace tests
}  // namespace gdl
