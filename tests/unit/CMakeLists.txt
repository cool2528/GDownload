# 单元测试配置
cmake_minimum_required(VERSION 3.18)

# 添加 GTest（包含 GMock）
find_package(GTest CONFIG REQUIRED)

# 创建单元测试主程序
add_executable(gdownload_unit_tests
    main.cpp
    core/test_core_functions.cpp
    utils/test_relaunch_function.cpp
    ${CMAKE_SOURCE_DIR}/src/App/ui/utils/utils.cxx
)

# 链接必要的库
target_link_libraries(gdownload_unit_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    GDLCore
)

# 包含目录
target_include_directories(gdownload_unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/App/ui
    ${CMAKE_SOURCE_DIR}/src/App/ui/Definitions
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)

target_compile_definitions(gdownload_unit_tests
    PRIVATE
    GDL_UTILS_DISABLE_QML_INTEGRATION
)

set_target_properties(gdownload_unit_tests PROPERTIES
    AUTOMOC ON
)

# 添加测试到 CTest
add_test(NAME UnitTests COMMAND gdownload_unit_tests)

# 设置测试属性
set_tests_properties(UnitTests PROPERTIES
    TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Debug/bin/Debug
)

# 注册目标以便清理
register_target(gdownload_unit_tests)
