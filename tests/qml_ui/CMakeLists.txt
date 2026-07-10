cmake_minimum_required(VERSION 3.18)

# QML UI 测试设施:视觉用例(quicktest)+ 集成用例(QtTest C++)+ 共享支持库
# 使用说明与已知限制见 tests/qml_ui/README.md
find_package(Qt6 REQUIRED COMPONENTS Test QuickTest Qml Quick)

add_subdirectory(support)
add_subdirectory(visual)
add_subdirectory(integration)

# fixture SETUP:测试前清空截图产物目录
add_test(NAME qml_ui_clean_artifacts
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/test_artifacts/qml_ui)
set_tests_properties(qml_ui_clean_artifacts PROPERTIES
    FIXTURES_SETUP qml_ui_artifacts
    LABELS qml_ui)

# fixture CLEANUP:测试结束后把 manifest.jsonl 折叠为 manifest.json
# 不设 WORKING_DIRECTORY:Phase 1 smoke 不写产物,目录可能不存在;
# finalize_manifest.cmake 用绝对路径 + if(EXISTS) 守卫,无产物时为空操作
# 注意:`cmake -P` 脚本模式下 CMAKE_BINARY_DIR 未定义,通过 ENVIRONMENT 把
# QML_UI_ARTIFACT_DIR 注入给 finalize_manifest.cmake 用作产物目录绝对路径
add_test(NAME qml_ui_finalize_manifest
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/finalize_manifest.cmake)
set_tests_properties(qml_ui_finalize_manifest PROPERTIES
    FIXTURES_CLEANUP qml_ui_artifacts
    LABELS qml_ui
    ENVIRONMENT "QML_UI_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test_artifacts/qml_ui")

# 聚合全部 QML UI 测试二进制，保证便利目标在全新构建目录中也可直接使用。
add_custom_target(qml_ui_test_binaries
    DEPENDS
        qml_ui_visual
        qml_ui_integration_smoke
        qml_ui_tst_save_settings
        qml_ui_tst_create_task
        qml_ui_tst_close_confirm
        qml_ui_tst_theme_toggle
        qml_ui_tst_navigation
        qml_ui_tst_design_system_contract)

# 一键跑全部 qml_ui 测试的便利目标
add_custom_target(run_qml_ui_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -C $<CONFIG> -L qml_ui --output-on-failure
    DEPENDS qml_ui_test_binaries
    COMMENT "Running QML UI tests")
