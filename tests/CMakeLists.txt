# 测试框架配置
cmake_minimum_required(VERSION 3.18)

# 查找必要的包
find_package(GTest CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

# 启用 CTest
enable_testing()

# 包含测试子目录
add_subdirectory(unit)
add_subdirectory(integration)

# 添加自定义测试目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -C $<CONFIG> --output-on-failure
    DEPENDS gdownload_unit_tests gdownload_logger_tests gdownload_cache_tests gdownload_aria2_lifecycle_tests gdownload_integration_tests
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
