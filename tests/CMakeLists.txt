# 测试框架配置
cmake_minimum_required(VERSION 3.18)

# 注意: 本目录树**不能**关闭 vcpkg applocal(set(VCPKG_APPLOCAL_DEPS OFF))。
# 这里的用例链接 vcpkg 的共享库 gtest/gmock(gtest.dll / gtest_main.dll / gmock.dll),
# 以及 sqlite3.dll、libcrypto-3-x64.dll —— 这些主程序 gdownload 并不导入, 因此不会被
# 主程序的 applocal 复制到共享输出目录; 一旦关闭本目录的 applocal, 这些 DLL 就没人复制,
# 测试进程会在加载阶段弹模态错误框, 在无人值守的 CI 上表现为「零输出超时」(已实际踩到:
# 6 个 gtest 用例全部 ***Timeout, 而同批产物里 19 个 QtTest 用例正常通过)。
# tests/qml_ui 可以安全关闭 —— 它的额外依赖只有 Qt 测试 DLL, 且已由 qml_ui_test_dlls
# 目标统一部署, 而那里正是并行 z-applocal 竞态的发生地。详见 tests/qml_ui/CMakeLists.txt。

# 查找必要的包
find_package(GTest CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

# 启用 CTest
enable_testing()

# 包含测试子目录
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(js_plugin)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME UpdateManifestScriptTests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/scripts/test_generate_update_manifest.py
    )
    set_tests_properties(UpdateManifestScriptTests PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# 添加自定义测试目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -C $<CONFIG> --output-on-failure
    DEPENDS gdownload_unit_tests gdownload_logger_tests gdownload_cache_tests gdownload_aria2_lifecycle_tests gdownload_update_trust_tests gdownload_integration_tests
    COMMENT "Running all tests with detailed output"
)

# 添加覆盖率测试（如果支持）
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_COMPILER_IS_GNUCXX)
    find_program(GCOV_PATH gcov)
    if(GCOV_PATH)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -C $<CONFIG> --output-on-failure
            COMMAND lcov --capture --directory . --output-file coverage.info
            COMMAND genhtml coverage.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests and generating coverage report"
        )
    endif()
endif()

# 测试清理目标
add_custom_target(clean_tests
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/Testing
    COMMENT "Cleaning test results"
)
