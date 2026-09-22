# 测试框架配置
cmake_minimum_required(VERSION 3.18)

# 关闭本目录树内所有测试目标的 vcpkg applocal, 必须在 add_subdirectory 之前设置
# (vcpkg toolchain 覆写的 add_executable 在创建目标时读取该变量)。
# 目的: applocal 是挂在每个可执行目标上的 POST_BUILD 拷贝命令, 并行构建时多个测试
# 目标会同时向同一输出目录复制同一批依赖 DLL, 触发 permission denied 竞态。
# 测试运行所需的 vcpkg/Qt DLL 由主程序 gdownload 部署到同一共享输出目录,
# ctest 在全量构建完成后运行, 届时 DLL 已就绪。详见 tests/qml_ui/CMakeLists.txt 的说明。
set(VCPKG_APPLOCAL_DEPS OFF)

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
