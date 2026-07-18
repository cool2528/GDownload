# JS 插件真实冒烟测试宿主（独立可执行，不依赖 Qt）
add_executable(js_plugin_smoke smoke_main.cxx)

target_link_libraries(js_plugin_smoke
    PRIVATE
        PluginManager
        GDLCore
)

target_include_directories(js_plugin_smoke
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/Module/plugin/PluginManager
        ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)

set_target_properties(js_plugin_smoke PROPERTIES FOLDER "Tests")
register_target(js_plugin_smoke)

# 插件市场服务冒烟测试
add_executable(market_smoke market_smoke_main.cxx)
target_link_libraries(market_smoke PRIVATE PluginManager GDLCore)
target_include_directories(market_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Module/plugin/PluginManager
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)
set_target_properties(market_smoke PROPERTIES FOLDER "Tests")
register_target(market_smoke)

# 插件配置系统单元测试（manifest settings / config store / gdl.config）
add_executable(plugin_config_test plugin_config_test.cxx)
target_link_libraries(plugin_config_test PRIVATE PluginManager GDLCore)
target_include_directories(plugin_config_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Module/plugin/PluginManager
    ${CMAKE_SOURCE_DIR}/src/Module/GDLCore
)
set_target_properties(plugin_config_test PROPERTIES FOLDER "Tests")
register_target(plugin_config_test)

