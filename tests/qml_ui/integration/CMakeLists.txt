# 集成用例:QtTest C++ 框架
# Phase 2 smoke:验证 qml_ui_integration_smoke 可执行文件 + offscreen 平台可用
# Phase 4 起追加 tst_save_settings / tst_create_task / tst_close_confirm /
# tst_theme_toggle / tst_navigation(各自独立可执行文件)
qt_add_executable(qml_ui_integration_smoke tst_smoke.cpp)
target_link_libraries(qml_ui_integration_smoke PRIVATE
    Qt6::Test
    qml_ui_support
)
target_compile_features(qml_ui_integration_smoke PRIVATE cxx_std_20)
# 显式启用 AUTOMOC:tst_smoke.cpp 内 TstSmoke 含 Q_OBJECT,
# 需 moc 生成 tst_smoke.moc 供 #include "tst_smoke.moc" 使用
set_target_properties(qml_ui_integration_smoke PROPERTIES AUTOMOC ON)
# 关闭 vcpkg applocal:运行时 DLL 由主程序部署到同一共享目录,避免并行 z-applocal 竞争
# 测试所需 Qt DLL 由父级 qml_ui_test_dlls 目标统一部署一次
set_target_properties(qml_ui_integration_smoke PROPERTIES VS_GLOBAL_VcpkgApplocalDeps false)

add_test(NAME qml_ui_integration_smoke
    COMMAND qml_ui_integration_smoke -platform offscreen)
set_tests_properties(qml_ui_integration_smoke PROPERTIES
    LABELS qml_ui
    ENVIRONMENT "GDOWNLOAD_TEST=1")

# ============================================================================
# Phase 4 集成用例:5 个独立 QtTest 可执行文件
#
# 每个用例:
#   - 经 qmlRegisterSingletonInstance 注入记录版 Fake(FakeBrowserManager /
#     FakeSettingsManager)+ TestStubs 桩单例,由 IntegrationHelper 统一设置
#   - 加载真实 QML 页面(qrc:/qml/...),findChild 按 objectName 定位控件,
#     模拟点击,QTRY_VERIFY/QCOMPARE 断言 Fake 调用历史
#   - 资源:复用主程序 resource_qml.qrc + resource_icons.qrc(AUTORCC 编译为
#     qrc_*.cpp 链入二进制,使 qrc:/qml/... 路径可解析)
#   - QT_QUICK_CONTROLS_STYLE=Basic:GButton/GCheckBox 等自定义控件依赖可定制样式,
#     Windows 默认样式不支持 background/contentItem 自定义,offscreen 平台兼容性差
# ============================================================================
set(QML_UI_INTEGRATION_TESTS
    save_settings
    create_task
    bulk_delete
    single_delete
    download_failure
    download_lifecycle
    close_confirm
    theme_toggle
    navigation
    design_system_contract
    surface_contract
    aurora_input_controls_contract
    aurora_icon_contract
    runtime_behavior
    ed2k_center
)

set(QML_UI_APP_RESOURCES
    ${CMAKE_SOURCE_DIR}/src/App/ui/Resource/resource_qml.qrc
    ${CMAKE_SOURCE_DIR}/src/App/ui/Resource/resource_icons.qrc
)

foreach(name ${QML_UI_INTEGRATION_TESTS})
    qt_add_executable(qml_ui_tst_${name}
        tst_${name}.cpp
        ${QML_UI_APP_RESOURCES}
    )
    target_link_libraries(qml_ui_tst_${name} PRIVATE
        Qt6::Test
        Qt6::Quick
        qml_ui_support
    )
    target_compile_features(qml_ui_tst_${name} PRIVATE cxx_std_20)
    # 暴露源码根路径给契约测试(SOURCE_ROOT 宏),用于按文件路径读取生产 QML 做静态断言
    target_compile_definitions(qml_ui_tst_${name} PRIVATE SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
    # AUTOMOC:测试类含 Q_OBJECT;AUTORCC:resource_qml.qrc / resource_icons.qrc
    # 编译为 qrc_*.cpp 链入二进制,使 qrc:/qml/... 路径可解析
    set_target_properties(qml_ui_tst_${name} PROPERTIES AUTOMOC ON AUTORCC ON)
    # 关闭 vcpkg applocal(见 smoke 处说明);Qt 测试 DLL 由父级 qml_ui_test_dlls 统一部署
    set_target_properties(qml_ui_tst_${name} PROPERTIES VS_GLOBAL_VcpkgApplocalDeps false)

    add_test(NAME qml_ui_tst_${name}
        COMMAND qml_ui_tst_${name} -platform offscreen
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(qml_ui_tst_${name} PROPERTIES
        LABELS qml_ui
        ENVIRONMENT "GDOWNLOAD_TEST=1;QT_QUICK_CONTROLS_STYLE=Basic")
endforeach()

# ============================================================================
# ed2k_center 额外依赖:Ed2kCenterPage.qml 内嵌的 Ed2kSearchPage/Ed2kServerPage 在
# 属性绑定与 Component.onCompleted 中直接调用真实 Ed2kManager 单例的方法(见
# tst_ed2k_center.cpp 顶部注释),故需把生产代码里的 Ed2kManager 及其依赖的两个
# QAbstractListModel 编译进本测试目标,并链接 Ed2kEngine(引擎层单例
# engine::Ed2kDownloadManager 所在的共享库,未 InitEd2kEngine 时全部方法空转安全)
# 与 GDLCore(LOG_WARN/LOG_ERR 等日志宏的实现)。其余 13 个测试不受影响。
# ============================================================================
find_package(nlohmann_json CONFIG REQUIRED)
target_sources(qml_ui_tst_ed2k_center PRIVATE
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_manager.h
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_manager.cxx
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_search_result_model.h
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_search_result_model.cxx
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_server_list_model.h
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k/ed2k_server_list_model.cxx
)
target_include_directories(qml_ui_tst_ed2k_center PRIVATE
    ${CMAKE_SOURCE_DIR}/src/App/ui/Ed2k
    ${CMAKE_SOURCE_DIR}/src/App/Ed2kEngine
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)
target_link_libraries(qml_ui_tst_ed2k_center PRIVATE
    Ed2kEngine
    GDLCore
    nlohmann_json::nlohmann_json
)
