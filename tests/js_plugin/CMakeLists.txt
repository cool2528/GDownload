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
