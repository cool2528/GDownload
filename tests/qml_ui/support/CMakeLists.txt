# qml_ui_support:测试期共享设施静态库
# - ScreenshotHelper:QML 端通过 Screenshot.capture(item, tag) 触发截图
# - FakeBrowserManager / FakeSettingsManager:继承 Impl 的测试替身(Phase 4 启用)
# - TestStubs:Phase 3 视觉用例桩对象(TestGTheme / TestBrowserManager 等),
#   在主项目重构为库之前(Task 9)替代真实单例,使页面可加载可渲染
# - PageHarness.qml:单页加载外壳(经 qrc 注册到 /tests/qml_ui/support/)
qt_add_library(qml_ui_support STATIC
    ScreenshotHelper.h
    ScreenshotHelper.cxx
    FakeBrowserManager.h
    FakeBrowserManager.cxx
    FakeSettingsManager.h
    FakeSettingsManager.cxx
    IntegrationHelper.h
    IntegrationHelper.cxx
    TestStubs.h
    TestStubs.cxx
    # 枚举头文件:AUTOMOC 处理 Q_NAMESPACE + Q_ENUM_NS + QML_NAMED_ELEMENT,
    # 生成静态初始化器把 GThemeType / SegoeFluentIcons 自动注册到 "gdl.sdk" 模块,
    # 使 QML 端 `GThemeType.ThemeMode.Dark` 等枚举引用可解析
    ${CMAKE_SOURCE_DIR}/src/App/ui/Definitions/appDef.h
    ${CMAKE_SOURCE_DIR}/src/App/ui/Definitions/fluentEnumDef.h
)

# 包含路径:
# - 本目录(support/):Fake / TestStubs 头文件互引
# - src/App/ui:Parser/file_preview_model.h 等
# - src/App/ui/Browser:browser_manager.h / IBrowserManager.h / download_task_model.h
# - src/App/ui/Settings:settings_manager.h / ISettings.h / setting.h
# - src/App/ui/Definitions:appDef.h(GThemeType 枚举) / fluentEnumDef.h(SegoeFluentIcons)
# - src/App/engine:Aria2CManager/ 头文件(browser_manager.h 传递依赖)
target_include_directories(qml_ui_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/App/ui
    ${CMAKE_SOURCE_DIR}/src/App/ui/Browser
    ${CMAKE_SOURCE_DIR}/src/App/ui/Settings
    ${CMAKE_SOURCE_DIR}/src/App/ui/Definitions
    ${CMAKE_SOURCE_DIR}/src/App/engine
)

# Fake 类继承 Impl,Impl 头文件传递依赖以下 vcpkg 包(编译期需要 include 路径):
#   - rapidjson(aria2c_http_rpc_client.h / aria2c_websocket_rpc_client.h)
#   - nlohmann_json(setting.h)
#   - boost_asio + boost_beast(websocket_client.h)
# 静态库不解析符号,但 moc 与源文件编译需这些头文件路径
find_package(RapidJSON CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_beast CONFIG REQUIRED)

target_link_libraries(qml_ui_support
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Test
    PRIVATE
        rapidjson
        nlohmann_json::nlohmann_json
        Boost::asio
        Boost::beast
)

target_compile_features(qml_ui_support PUBLIC cxx_std_20)

# 显式启用 AUTOMOC:ScreenshotHelper / FakeBrowserManager / FakeSettingsManager
# 均含 Q_OBJECT,需 moc 生成元对象实现(metaObject / qt_metacast / qt_metacall)
set_target_properties(qml_ui_support PROPERTIES AUTOMOC ON)

# PageHarness.qml / 预览外壳 / GMessageBoxMulti.qml 注册到 qrc,供视觉用例 Loader 加载
# - PageHarness.qml:单页加载外壳(主题切换 + Loader)
# - GMessageBoxMulti.qml:GMessageBox 4-type 横向并排包装器(tst_dialogs 用)
qt_add_resources(qml_ui_support "qml_ui_support_qml"
    PREFIX "/tests/qml_ui/support"
    FILES PageHarness.qml
          ApplicationShellPreview.qml
          NavigatorPreview.qml
          GMessageBoxMulti.qml
)
