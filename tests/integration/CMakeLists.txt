# 集成测试配置
cmake_minimum_required(VERSION 3.18)

# 添加 GTest（包含 GMock）
find_package(GTest CONFIG REQUIRED)

# 创建集成测试主程序
add_executable(gdownload_integration_tests
    main.cpp
    test_relaunch_simple.cpp
    test_relaunch_after_exit.cpp
    ${CMAKE_SOURCE_DIR}/src/App/ui/utils/utils.cxx
)

# 链接必要的库
target_link_libraries(gdownload_integration_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    GDLCore
)

# 包含目录
target_include_directories(gdownload_integration_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/App/ui/utils
    ${CMAKE_SOURCE_DIR}/src/App/ui/Definitions
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)

target_compile_definitions(gdownload_integration_tests
    PRIVATE
    GDL_UTILS_DISABLE_QML_INTEGRATION
)

set_target_properties(gdownload_integration_tests PROPERTIES
    AUTOMOC ON
)

# 添加测试到 CTest
add_test(NAME IntegrationTests COMMAND gdownload_integration_tests)

# 设置测试属性（集成测试可能需要更长时间）
set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 600
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Debug/bin/Debug
)

# 注册目标
register_target(gdownload_integration_tests)
