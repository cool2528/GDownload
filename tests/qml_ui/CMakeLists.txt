cmake_minimum_required(VERSION 3.18)

# QML UI 测试设施:视觉用例(quicktest)+ 集成用例(QtTest C++)+ 共享支持库
# 使用说明与已知限制见 tests/qml_ui/README.md
find_package(Qt6 REQUIRED COMPONENTS Test QuickTest Qml Quick)

add_subdirectory(support)
add_subdirectory(visual)
add_subdirectory(integration)

# ============================================================================
# 统一一次性部署测试所需 Qt DLL(Qt6Test / Qt6QuickTest / offscreen 平台插件)到
# 共享输出目录。原先每个测试目标各自 POST_BUILD 拷贝同一批 DLL 到同一目录,清洁并行
# 构建时多进程争抢同一目标文件导致文件锁失败(MSB3073)。改为单目标部署一次、所有测试
# 目标依赖它,彻底消除竞争。vcpkg 的 z-applocal 竞争则通过在各测试目标关闭 applocal
# 解决(vcpkg/Qt 运行时 DLL 由主程序 gdownload 部署到同一目录,测试经 ctest 在全量
# 构建完成后运行,届时 DLL 已就绪)。
# ============================================================================
get_filename_component(QT_TEST_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
get_filename_component(QT_TEST_PLATFORMS_DIR "${Qt6_DIR}/../../../plugins/platforms" ABSOLUTE)
# 用静态运行时输出目录(不引用具体 target,避免与"测试目标依赖本部署目标"形成依赖环)
set(_qml_ui_out "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
add_custom_target(qml_ui_test_dlls
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_qml_ui_out}/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_TEST_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
        "${_qml_ui_out}/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_TEST_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6QuickTestd.dll,Qt6QuickTest.dll>"
        "${_qml_ui_out}/$<IF:$<CONFIG:Debug>,Qt6QuickTestd.dll,Qt6QuickTest.dll>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_TEST_PLATFORMS_DIR}/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
        "${_qml_ui_out}/platforms/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
    VERBATIM
    COMMENT "Deploying Qt test DLLs once (shared) for qml_ui tests")

# 所有测试目标依赖统一部署目标:并行构建时部署只跑一次,无文件锁竞争
set(QML_UI_ALL_TEST_TARGETS
    qml_ui_visual
    qml_ui_integration_smoke
    qml_ui_tst_save_settings
    qml_ui_tst_create_task
    qml_ui_tst_bulk_delete
    qml_ui_tst_single_delete
    qml_ui_tst_download_failure
    qml_ui_tst_download_lifecycle
    qml_ui_tst_close_confirm
    qml_ui_tst_theme_toggle
    qml_ui_tst_navigation
    qml_ui_tst_design_system_contract
    qml_ui_tst_surface_contract
    qml_ui_tst_aurora_input_controls_contract
    qml_ui_tst_aurora_icon_contract
    qml_ui_tst_runtime_behavior
    qml_ui_tst_ed2k_center)
foreach(_t ${QML_UI_ALL_TEST_TARGETS})
    add_dependencies(${_t} qml_ui_test_dlls)
endforeach()

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
        qml_ui_tst_bulk_delete
        qml_ui_tst_single_delete
        qml_ui_tst_download_failure
        qml_ui_tst_download_lifecycle
        qml_ui_tst_close_confirm
        qml_ui_tst_theme_toggle
        qml_ui_tst_navigation
        qml_ui_tst_design_system_contract
        qml_ui_tst_surface_contract
        qml_ui_tst_aurora_input_controls_contract)

# 一键跑全部 qml_ui 测试的便利目标
add_custom_target(run_qml_ui_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -C $<CONFIG> -L qml_ui --output-on-failure
    DEPENDS qml_ui_test_binaries
    COMMENT "Running QML UI tests")
