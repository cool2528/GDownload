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

# 在项目开始处添加
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 然后使用不带后缀的变量
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

# 设置通用输出目录（会被具体的 Debug/Release 设置覆盖）
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

# 引入cmake
include(${CMAKE_SOURCE_DIR}/cmake/shareLibraryDefined.cmake)

# 获取版本信息
include(${CMAKE_SOURCE_DIR}/cmake/get_version.cmake)
get_version_info(MAJOR MINOR BUILD COMMIT)
message(STATUS "MAJOR:${MAJOR}")
message(STATUS "MINOR:${MINOR}")
message(STATUS "BUILD:${BUILD}")
message(STATUS "COMMIT:${COMMIT}")

# 设置版本信息
set(CURRENT_VERSION_MAJOR ${MAJOR})
set(CURRENT_VERSION_MINOR ${MINOR})
set(CURRENT_VERSION_BUILD ${BUILD})
set(CURRENT_VERSION_COMMIT ${COMMIT})
set(APP_VERSION "${CURRENT_VERSION_MAJOR}.${CURRENT_VERSION_MINOR}.${CURRENT_VERSION_BUILD}")

# 设置vcpkg的manifest模式
set(VCPKG_MANIFEST_MODE ON)

# 生成版本头文件
configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_SOURCE_DIR}/src/version.h)

# 设置依赖库的根路径
if(APPLE)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/osx")

    # 查询系统芯片架构
    cmake_host_system_information(RESULT OSX_ARCHITECTURES QUERY OS_PLATFORM)
    message(STATUS "OSX_ARCHITECTURES:${OSX_ARCHITECTURES}")
    set(CMAKE_OSX_ARCHITECTURES ${OSX_ARCHITECTURES})

elseif(WIN32)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/win")

    # 当使用visual studio 以文件夹方式打开cmake 项目通过 QT visual studio tool 生成的cmake 预设时，设置编译选项
    if(CMAKE_GENERATOR STREQUAL "Ninja" OR CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
        add_compile_options(/EHsc)
    endif()

    # 设置Windows目标版本
    add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10
elseif(UNIX AND NOT APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
    set(CMAKE_VISIBILITY_INLINES_HIDDEN "YES")
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/linux")
endif()

project(GDownload LANGUAGES CXX C)
message(STATUS "CMAKE_PREFIX_PATH:${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_TOOLCHAIN_FILE:${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
set(MODULE_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/src/Module")
set(CORE_INCLUDE_PATH ${MODULE_INCLUDE_PATH}/GDLCore)
set(PLUGIN_INCLUDE_PATH ${MODULE_INCLUDE_PATH}/plugin)
set(PLUGIN_MANAGER_INCLUDE ${PLUGIN_INCLUDE_PATH}/PluginManager)
include_directories(${CMAKE_SOURCE_DIR}/src
    ${MODULE_INCLUDE_PATH}
    ${CORE_INCLUDE_PATH}
    ${PLUGIN_INCLUDE_PATH}
    ${PLUGIN_MANAGER_INCLUDE}
    ${CMAKE_BINARY_DIR}/src
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(src)

# 安装规则
if(WIN32)
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)

    # Windows 安装规则
    install(TARGETS ${PROJECT_TARGETS}
        RUNTIME DESTINATION .
        LIBRARY DESTINATION .
        ARCHIVE DESTINATION .
    )
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/extra/win32/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,ia32>/engine/ DESTINATION engine)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/$<IF:$<CONFIG:Debug>,debug,release>/bin/ DESTINATION .)
    install(FILES README.md LICENSE.txt DESTINATION .)

    # 设置默认安装前缀为程序文件目录
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/GDownload" CACHE PATH "Installation prefix" FORCE)
    endif()
elseif(APPLE)
    # macOS 安装规则
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)
    install(TARGETS ${PROJECT_TARGETS}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
        RUNTIME DESTINATION ${PROJECT_NAME}.app/Contents/MacOS
    )

    # 安装第三方依赖的动态库到Frameworks目录
    file(GLOB FRAMEWORK_DYLIBS "${DEPENDENCIES_ROOT_PATH}/framelesshelper/lib/*.dylib")
    install(FILES ${FRAMEWORK_DYLIBS}
        DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
    )

    # 运行 macdeployqt
    install(CODE "
        execute_process(
            COMMAND ${Qt6_DIR}/../../../bin/macdeployqt 
                    \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app
                    -verbose=3
                    -qmlimport=${Qt6_DIR}/../../../qml
                    -qmldir=${CMAKE_SOURCE_DIR}/src/App/ui/Resource/qml
                    -always-overwrite
            RESULT_VARIABLE MACDEPLOYQT_RESULT
        )
        if(NOT MACDEPLOYQT_RESULT EQUAL 0)
            message(FATAL_ERROR \"macdeployqt failed with error: \${MACDEPLOYQT_RESULT}\")
        endif()
    ")

    # 复制额外的Qt插件（如果有特定需求）
    install(CODE "
        file(GLOB QT_PLUGINS
            \"${Qt6_DIR}/../../../plugins/platforms/*.dylib\"
            \"${Qt6_DIR}/../../../plugins/styles/*.dylib\"
        )
        foreach(PLUGIN \${QT_PLUGINS})
            file(COPY \${PLUGIN} DESTINATION \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns)
        endforeach()
    ")

    # 添加签名步骤
    install(CODE "
       execute_process(
           COMMAND codesign --force --deep --sign - 
                   --timestamp=none
                   --options runtime
                   --entitlements ${CMAKE_SOURCE_DIR}/package/entitlements.plist
                   \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app
           RESULT_VARIABLE CODESIGN_RESULT
       )
       if(NOT CODESIGN_RESULT EQUAL 0)
           message(WARNING \"Code signing failed with error: \${CODESIGN_RESULT}\")
       endif()
   ")

    # 创建 DMG
    install(CODE "
        # 确保 create-dmg 已安装
        execute_process(
            COMMAND which create-dmg
            RESULT_VARIABLE CREATE_DMG_EXISTS
        )
        if(NOT CREATE_DMG_EXISTS EQUAL 0)
            message(FATAL_ERROR \"Please install create-dmg: brew install create-dmg\")
        endif()

        # 设置 DMG 相关变量
        set(DMG_FILENAME \"${PROJECT_NAME}-${APP_VERSION}.dmg\")
        set(DMG_TEMP_DIR \"\${CMAKE_INSTALL_PREFIX}/dmg\")
        
        # 检查应用图标是否存在
        if(NOT EXISTS \"${CMAKE_SOURCE_DIR}/package/dmg_icon.icns\")
            message(WARNING \"DMG icon not found, will use default icon\")
        endif()

        # 创建临时目录
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory \"\${DMG_TEMP_DIR}\"
        )

        # 复制应用到临时目录
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app
                    \"\${DMG_TEMP_DIR}/${PROJECT_NAME}.app\"
            RESULT_VARIABLE COPY_RESULT
        )
        if(NOT COPY_RESULT EQUAL 0)
            message(FATAL_ERROR \"Failed to copy application to DMG directory\")
        endif()

        # 创建 DMG（使用简化的参数）
        execute_process(
            COMMAND create-dmg
                    --volname \"${PROJECT_NAME}\"
                    --volicon \"${CMAKE_SOURCE_DIR}/package/dmg_icon.icns\"
                    --window-pos 200 120
                    --window-size 600 400
                    --icon-size 128
                    --icon \"${PROJECT_NAME}.app\" 140 150
                    --app-drop-link 460 150
                    --no-internet-enable
                    \"\${CMAKE_INSTALL_PREFIX}/\${DMG_FILENAME}\"
                    \"\${DMG_TEMP_DIR}\"
            RESULT_VARIABLE CREATE_DMG_RESULT
        )

        if(NOT CREATE_DMG_RESULT EQUAL 0)
            message(FATAL_ERROR \"Failed to create DMG: \${CREATE_DMG_RESULT}\")
        endif()

        # 清理临时目录
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E remove_directory \"\${DMG_TEMP_DIR}\"
        )

        message(STATUS \"Created DMG: \${CMAKE_INSTALL_PREFIX}/\${DMG_FILENAME}\")
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
