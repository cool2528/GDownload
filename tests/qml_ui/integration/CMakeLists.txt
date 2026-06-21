# 集成用例:QtTest C++ 框架
# Phase 2 smoke:验证 qml_ui_integration_smoke 可执行文件 + offscreen 平台可用
# Phase 4 起追加 tst_save_settings / tst_create_task / tst_close_confirm /
# tst_theme_toggle / tst_navigation
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
