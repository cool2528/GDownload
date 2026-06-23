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

# 复制测试所需的 Qt DLL 与 offscreen 平台插件到可执行文件目录
get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
get_filename_component(QT_PLATFORMS_DIR "${Qt6_DIR}/../../../plugins/platforms" ABSOLUTE)
add_custom_command(TARGET qml_ui_integration_smoke POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
        "$<TARGET_FILE_DIR:qml_ui_integration_smoke>/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_PLATFORMS_DIR}/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
        "$<TARGET_FILE_DIR:qml_ui_integration_smoke>/platforms/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
    COMMENT "Copying Qt Test DLL and offscreen platform plugin for headless testing")

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
    close_confirm
    theme_toggle
    navigation
    design_system_contract
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

    # 复制 Qt6Testd.dll + qoffscreend.dll 到可执行文件目录(主程序 windeployqt 不复制 QtTest)
    add_custom_command(TARGET qml_ui_tst_${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
            "$<TARGET_FILE_DIR:qml_ui_tst_${name}>/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLATFORMS_DIR}/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
            "$<TARGET_FILE_DIR:qml_ui_tst_${name}>/platforms/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
        COMMENT "Copying Qt Test DLL and offscreen platform plugin for ${name}")

    add_test(NAME qml_ui_tst_${name}
        COMMAND qml_ui_tst_${name} -platform offscreen
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(qml_ui_tst_${name} PROPERTIES
        LABELS qml_ui
        ENVIRONMENT "GDOWNLOAD_TEST=1;QT_QUICK_CONTROLS_STYLE=Basic")
endforeach()
