# 测试框架配置
cmake_minimum_required(VERSION 3.18)

# ============================================================================
# 关闭本目录树内所有测试目标的 vcpkg applocal。
#
# 必须在 add_subdirectory 之前设置: vcpkg toolchain 覆写的 add_executable 在创建目标
# 时读取该变量, 而它是目录作用域变量, 会自动传递给子目录(已用最小工程实测确认)。
#
# 为什么必须关: applocal 是挂在每个可执行目标上的 POST_BUILD 拷贝命令。并行构建时多个
# 测试目标会同时向同一输出目录复制同一批依赖 DLL, 报:
#   open_for_read(".../build/Debug/bin\Qt6Guid.dll"): permission denied
# 实测这个竞态在 gtest 用例之间同样会触发(CI 上曾命中 unit_tests + integration_tests),
# 因此整个 tests/ 目录树都必须关闭, 不能只关 qml_ui。
#
# 关闭后缺失的依赖由谁补:
#   - sqlite3.dll / libcrypto-3-x64.dll / 各 Qt DLL: 由 src/ 目标的 applocal 复制到同一
#     共享输出目录(实测 GDLCore.dll 导入 sqlite3.dll, Ed2kEngine/GDLCore/PluginManager
#     导入 libcrypto-3-x64.dll, 主程序导入各 Qt DLL), 无需重复部署;
#   - gtest.dll / gtest_main.dll / gmock.dll: 没有任何 src 目标导入它们, 所以必须由下方
#     deploy_test_runtime_dlls 目标显式部署 —— 它是这三个文件的唯一写入者, 天然没有竞态。
# 历史教训: 曾经漏掉最后一步, 导致 6 个 gtest 用例在加载阶段弹模态错误框, 在无人值守的
# CI 上表现为「零输出超时」(ctest --output-on-failure 也没有任何输出)。
# ============================================================================
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

# ============================================================================
# 部署测试专属的 vcpkg 运行时 DLL。
#
# 这些 DLL 主程序 gdownload 及其任何自有库都不导入(已用 dumpbin /dependents 逐一确认),
# 因此不会被 src/ 目标的 applocal 带到共享输出目录; 而本目录树的 applocal 已关闭(见上),
# 所以必须在这里显式部署一次。放在单个自定义目标里执行 = 只有这一个写入者, 不会与任何
# applocal 争抢同一文件, 也就不会再制造出它要解决的问题。
#
# 若将来新增用例引入了别的 vcpkg 依赖 DLL, 症状是测试进程加载阶段弹模态错误框、CI 上
# 表现为零输出超时; 届时重做一次差集分析(dumpbin /dependents 对比 gdownload.exe)把新
# DLL 补进下面的清单即可。
# ============================================================================
if(WIN32 AND VCPKG_TARGET_TRIPLET)
    # vcpkg 安装树按配置分目录(Debug 在 debug/bin); gtest 系列在两种配置下同名
    set(_vcpkg_test_bin_dir
        "$<IF:$<CONFIG:Debug>,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin>")
    # 多配置生成器(VS)会在运行时输出目录后再追加一层 $<CONFIG>, 单配置(Ninja)不会
    if(CMAKE_CONFIGURATION_TYPES)
        set(_test_runtime_out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
    else()
        set(_test_runtime_out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
    add_custom_target(deploy_test_runtime_dlls ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_vcpkg_test_bin_dir}/gtest.dll"
            "${_vcpkg_test_bin_dir}/gtest_main.dll"
            "${_vcpkg_test_bin_dir}/gmock.dll"
            "${_test_runtime_out_dir}"
        COMMENT "Deploy vcpkg gtest runtime DLLs required by tests"
        VERBATIM)
endif()

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
