cmake_minimum_required(VERSION 3.18)
# 在项目根目录的 CMakeLists.txt 开始处添加
set_property(GLOBAL PROPERTY PROJECT_TARGETS "")

# 创建一个函数用于注册目标
function(register_target target)
    get_property(targets GLOBAL PROPERTY PROJECT_TARGETS)
    list(APPEND targets ${target})
    set_property(GLOBAL PROPERTY PROJECT_TARGETS "${targets}")
endfunction()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置通用输出目录（会被具体的 Debug/Release 设置覆盖）
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/bin)
# 引入cmake
include(${CMAKE_SOURCE_DIR}/cmake/shareLibraryDefined.cmake)

# 设置依赖库的根路径
if(APPLE)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/osx")
    # 查询系统芯片架构
    cmake_host_system_information(RESULT OSX_ARCHITECTURES QUERY  OS_PLATFORM)
    message(STATUS "OSX_ARCHITECTURES:${OSX_ARCHITECTURES}")
    set(CMAKE_OSX_ARCHITECTURES ${OSX_ARCHITECTURES})

elseif(WIN32)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/win")
elseif(UNIX AND NOT APPLE)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/linux")
endif()
project(GDownload LANGUAGES CXX C)
message(STATUS "CMAKE_PREFIX_PATH:${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_TOOLCHAIN_FILE:${CMAKE_TOOLCHAIN_FILE}")
set(MODULE_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/src/Module")
set(CORE_INCLUDE_PATH ${MODULE_INCLUDE_PATH}/GDLCore)
set(PLUGIN_INCLUDE_PATH ${MODULE_INCLUDE_PATH}/plugin)
set(PLUGIN_MANAGER_INCLUDE ${PLUGIN_INCLUDE_PATH}/PluginManager)
include_directories(${CMAKE_SOURCE_DIR}/src
    ${MODULE_INCLUDE_PATH}
    ${CORE_INCLUDE_PATH}
    ${PLUGIN_INCLUDE_PATH}
    ${PLUGIN_MANAGER_INCLUDE}
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(src)
# 安装规则
if(WIN32)
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)
    # Windows 安装规则
    install(TARGETS ${PROJECT_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/extra/win32/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,ia32>/engine/ DESTINATION engine)
    install(FILES README.md LICENSE.txt DESTINATION .)
    # 设置默认安装前缀为程序文件目录
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/GDownload" CACHE PATH "Installation prefix" FORCE)
    endif()
elseif(APPLE)
    # macOS 安装规则
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)
    install(TARGETS ${PROJECT_TARGETS}
        BUNDLE DESTINATION ${CMAKE_BINARY_DIR}
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks
        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS
        ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks
    )
    
    # 安装第三方依赖的动态库到Frameworks目录
    file(GLOB FRAMEWORK_DYLIBS "${DEPENDENCIES_ROOT_PATH}/framelesshelper/lib/*.dylib")
    install(FILES ${FRAMEWORK_DYLIBS}
        DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks
    )
    
    # 安装文档文件到Resources目录
    install(FILES README.md LICENSE.txt 
        DESTINATION ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources
    )
    
    # 添加安装后命令修复动态库路径
    install(CODE "
        include(BundleUtilities)
        # 获取 Qt 库路径
        execute_process(
            COMMAND ${Qt6_DIR}/../../../bin/qmake -query QT_INSTALL_LIBS
            OUTPUT_VARIABLE QT_LIBS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # 获取 Qt 插件路径
        execute_process(
            COMMAND ${Qt6_DIR}/../../../bin/qmake -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE QT_PLUGINS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(DIRS 
            \"\${QT_LIBS_DIR}\"
            \"\${QT_PLUGINS_DIR}\"
            \"${DEPENDENCIES_ROOT_PATH}/framelesshelper/lib\"
            \"${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Frameworks\"
            \"${CMAKE_BINARY_DIR}/debug/bin\"
            \"${CMAKE_BINARY_DIR}/release/bin\"
        )
        message(STATUS \"Looking for dependencies in: \${DIRS}\")
        fixup_bundle(
            \"\${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app\"
            \"\"
            \"\${DIRS}\"
        )
    ")
    
    # 设置默认安装前缀
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Installation prefix" FORCE)
    endif()
elseif(UNIX AND NOT APPLE)
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)
    # Linux 安装规则
    install(TARGETS ${PROJECT_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(FILES README.md LICENSE.txt DESTINATION share/GDownload)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/extra/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/engine DESTINATION engine)
    # 设置默认安装前缀为 /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix" FORCE)
    endif()
endif()
