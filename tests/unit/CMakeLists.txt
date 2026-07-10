# 单元测试配置
cmake_minimum_required(VERSION 3.18)

# 添加 GTest（包含 GMock）
find_package(GTest CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Qml)

# 创建单元测试主程序
add_executable(gdownload_unit_tests
    main.cpp
    core/test_core_functions.cpp
    engine/test_publish_subscribe_system.cpp
    update/test_update_config.cpp
    browser/test_download_url_utils.cpp
    browser/test_download_task_utils.cpp
    settings/test_aria2_defaults.cpp
    utils/test_relaunch_function.cpp
    ${CMAKE_SOURCE_DIR}/src/App/ui/Browser/download_task_model.cxx
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
    Qt6::Qml
    Qt6::Widgets
    GDLCore
    Boost::asio
)

if(WIN32)
    target_link_libraries(gdownload_unit_tests PRIVATE Advapi32)
endif()

# 包含目录
target_include_directories(gdownload_unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/App/ui
    ${CMAKE_SOURCE_DIR}/src/App/engine
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
    WORKING_DIRECTORY $<TARGET_FILE_DIR:gdownload_unit_tests>
)

# 注册目标以便清理
register_target(gdownload_unit_tests)

# 日志测试会关闭全局 spdlog 注册表，使用独立进程避免污染其它单元测试。
add_executable(gdownload_logger_tests
    core/test_logger_lifecycle.cpp
)
target_link_libraries(gdownload_logger_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    GDLCore
)
target_include_directories(gdownload_logger_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)
add_test(NAME CoreLoggerTests COMMAND gdownload_logger_tests)
set_tests_properties(CoreLoggerTests PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY $<TARGET_FILE_DIR:gdownload_logger_tests>
)
register_target(gdownload_logger_tests)
