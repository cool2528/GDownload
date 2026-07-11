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
# 设置vcpkg的overlay ports路径
set(VCPKG_OVERLAY_PORTS "${CMAKE_SOURCE_DIR}/vcpkg-ports")

# 生成版本头文件
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src)  # 确保输出目录存在（C1）
configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/src/version.h)  # 生成到构建目录，避免污染源码树（C1）
set(GDOWNLOAD_UPDATE_PUBLIC_KEY_BASE64 "" CACHE STRING "Ed25519 public key for signed update manifests")
set(GDOWNLOAD_UPDATE_SIGNER_SPKI_PIN "" CACHE STRING "SHA-256 SPKI pin for the Windows update signer")
option(GDOWNLOAD_REQUIRE_APPIMAGE_SIGNATURE "Require signed AppImage updates (OFF is migration-only)" OFF)
configure_file(${CMAKE_SOURCE_DIR}/update_trust_config.h.in
    ${CMAKE_BINARY_DIR}/src/update_trust_config.h @ONLY)

# 设置依赖库的根路径（仅保留 Sparkle 自动更新框架）
if(APPLE)
    set(SPARKLE_ROOT_PATH "${CMAKE_SOURCE_DIR}/Sparkle")
    # 使用自定义的 triplets
    set(VCPKG_OVERLAY_TRIPLETS  ${CMAKE_SOURCE_DIR}/custom-triplets)

    # 最低支持 macOS 11.0 (Big Sur)。Qt 6.5.2 最低要求 11.0；
    # 未显式指定时锁定 11.0，避免在更高 SDK (如 macOS 26 Tahoe) 上编译时默认取 SDK 版本，
    # 导致产物 LC_BUILD_VERSION 声明 minos 过高、旧系统 LaunchServices 拒绝启动 (error -10825)。
    # 可用 -DCMAKE_OSX_DEPLOYMENT_TARGET=12 覆盖。
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
    endif()
    message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")

    # macOS 架构配置
    # 支持三种模式：
    # 1. 手动指定单架构：-DCMAKE_OSX_ARCHITECTURES=arm64 或 x86_64
    # 2. 手动指定多架构（Universal Binary）：-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
    # 3. 自动检测当前主机架构（默认）
    if(DEFINED CMAKE_OSX_ARCHITECTURES)
        message(STATUS "Using specified macOS architectures: ${CMAKE_OSX_ARCHITECTURES}")
    else()
        cmake_host_system_information(RESULT OSX_ARCHITECTURES QUERY OS_PLATFORM)
        message(STATUS "Auto-detected macOS architecture: ${OSX_ARCHITECTURES}")
        set(CMAKE_OSX_ARCHITECTURES ${OSX_ARCHITECTURES})
    endif()

elseif(WIN32)
    # 使用自定义的 triplets（用于传递 Qt 路径给 vcpkg 构建的包）
    set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_SOURCE_DIR}/custom-triplets)
    # MSVC 使用 Ninja 生成器时需要显式开启异常处理，避免影响 MinGW 等非 MSVC 编译器
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/EHsc> $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/FS>)
    # 设置Windows目标版本
    add_compile_definitions(_WIN32_WINNT=0x0A00) # Windows 10
elseif(UNIX AND NOT APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
    set(CMAKE_VISIBILITY_INLINES_HIDDEN "YES")
    # 设置 RPATH
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(VCPKG_OVERLAY_TRIPLETS  ${CMAKE_SOURCE_DIR}/custom-triplets)
endif()

project(GDownload LANGUAGES CXX C)
option(GDL_UI_GENERATE_TRANSLATIONS "Generate TS/QM translation files during configure" OFF)
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
if(UNIX AND NOT APPLE)
    set(SAVED_CXX_VISIBILITY_PRESET ${CMAKE_CXX_VISIBILITY_PRESET})
    set(SAVED_VISIBILITY_INLINES_HIDDEN ${CMAKE_VISIBILITY_INLINES_HIDDEN})
    # 临时移除可见性限制
    unset(CMAKE_CXX_VISIBILITY_PRESET)
    unset(CMAKE_VISIBILITY_INLINES_HIDDEN)
    # 如果是Linux 系统使用 appimageupdate 更新 appimage
    set(BUILD_LIBAPPIMAGEUPDATE_ONLY ON CACHE BOOL "Only build libappimageupdate" FORCE)
    set(USE_SYSTEM_LIBAPPIMAGE ON CACHE BOOL "Use system libappimage" FORCE)
    add_subdirectory(lib/AppImageUpdate)
    # 恢复可见性设置
    set(CMAKE_CXX_VISIBILITY_PRESET ${SAVED_CXX_VISIBILITY_PRESET})
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ${SAVED_VISIBILITY_INLINES_HIDDEN})
endif()
add_subdirectory(src)

# 可选的测试支持
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    # QML UI 测试设施:视觉用例(quicktest)+ 集成用例(QtTest C++)
    # 使用说明与已知限制见 tests/qml_ui/README.md
    add_subdirectory(tests/qml_ui)
    add_dependencies(run_all_tests qml_ui_test_binaries)
endif()

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

    # 这里要复制 vcpkg 的依赖库到Frameworks目录
    file(GLOB VCPKG_LIBS 
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/*.dylib"
    )
    install(FILES ${VCPKG_LIBS}
        DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
    )
    # 先删除目标目录中可能存在的 Sparkle.framework
    install(CODE "
        if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/Sparkle.framework\")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E remove_directory 
                \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/Sparkle.framework\"
            )
        endif()
    ")
    
    # 然后复制整个框架
    install(DIRECTORY ${SPARKLE_ROOT_PATH}/Sparkle.framework
        DESTINATION ${PROJECT_NAME}.app/Contents/Frameworks
        USE_SOURCE_PERMISSIONS
        PATTERN ".DS_Store" EXCLUDE
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
        #删除所有调试符号文件
        execute_process(
            COMMAND find \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app -name \"*.dSYM\" -type d -exec rm -rf {} +
            COMMAND strip -S \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/*
            COMMAND find \${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks -type f -name \"*.dylib\" -exec strip -S {} +
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
    include(GNUInstallDirs)
    option(GDL_GENERATE_APPIMAGE "使用 linuxdeploy 生成 AppImage（仅限 Linux）" OFF)
    
    # 获取所有注册的目标
    get_property(PROJECT_TARGETS GLOBAL PROPERTY PROJECT_TARGETS)
    
    # 设置安装路径
    set(APP_NAME "gdownload")
    set(APP_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
    set(APP_BIN_DIR "${APP_INSTALL_DIR}/usr/bin") 
    set(APP_LIB_DIR "${APP_INSTALL_DIR}/usr/lib")
    set(APP_SHARE_DIR "${APP_INSTALL_DIR}/usr/share")
    # 安装所有目标
    install(TARGETS ${PROJECT_TARGETS}
        RUNTIME DESTINATION ${APP_BIN_DIR}
        LIBRARY DESTINATION ${APP_LIB_DIR}
        ARCHIVE DESTINATION ${APP_LIB_DIR}
    )

    # 复制 vcpkg 依赖的动态库
    file(GLOB VCPKG_LIBS 
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/*.so*"
    )
    install(FILES ${VCPKG_LIBS}
        DESTINATION ${APP_LIB_DIR}
    )

    # 复制 aria2c文件和配置文件
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/extra/linux/$<IF:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,x64,arm64>/engine/ 
        DESTINATION ${APP_BIN_DIR}/engine
        FILE_PERMISSIONS 
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    )
    # 复制translations 文件夹下的文件
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/App/ui/Resource/translations/
        DESTINATION  ${APP_BIN_DIR}/translations
    )
    # 创建和安装desktop文件
    configure_file(
        ${CMAKE_SOURCE_DIR}/package/${PROJECT_NAME}.desktop.in
        ${CMAKE_BINARY_DIR}/gdownload.desktop
        @ONLY
    )
    install(FILES ${CMAKE_BINARY_DIR}/gdownload.desktop
        DESTINATION ${APP_SHARE_DIR}/applications
    )
    
    # 安装图标文件
    install(FILES "${CMAKE_SOURCE_DIR}/src/App/ui/Resource/images/logo/logo.svg"
        DESTINATION ${APP_SHARE_DIR}/icons/hicolor/256x256/apps
        RENAME "gdownload.svg"
    )
    
    if(GDL_GENERATE_APPIMAGE)
        find_program(LINUXDEPLOY_PATH 
            NAMES 
                "linuxdeploy-x86_64.AppImage"
                "linuxdeploy"
            DOC "Path to linuxdeploy AppImage"
        )

        if(NOT LINUXDEPLOY_PATH)
            message(STATUS "LinuxDeploy not found in system")
        else()
            message(STATUS "Found LinuxDeploy: ${LINUXDEPLOY_PATH}")
            
            install(CODE "
                set(ENV{LD_LIBRARY_PATH} \"${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin:${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/usr/lib:${Qt6_DIR}/../../../lib:\$ENV{LD_LIBRARY_PATH}\")
                set(ENV{QML_SOURCES_PATHS} \"${CMAKE_SOURCE_DIR}/src/App/ui/Resource/qml\")
                set(ENV{QMAKE} \"${Qt6_DIR}/../../../bin/qmake\")
                set(ENV{QT_PLUGIN_PATH} \"${Qt6_DIR}/../../../plugins\")
                set(ENV{QML_IMPORT_PATH} \"${Qt6_DIR}/../../../qml\")
                set(ENV{UPDATE_INFORMATION} \"gh-releases-zsync|cool2528|GDownload|latest|gdownload-*.AppImage.zsync\")
                
                execute_process(
                    COMMAND \"${LINUXDEPLOY_PATH}\"
                            --appdir=${CMAKE_INSTALL_PREFIX}
                            --executable=${CMAKE_INSTALL_PREFIX}/usr/bin/gdownload
                            --desktop-file=${CMAKE_INSTALL_PREFIX}/usr/share/applications/gdownload.desktop
                            --icon-file=${CMAKE_INSTALL_PREFIX}/usr/share/icons/hicolor/256x256/apps/gdownload.svg
                            --plugin qt
                            --output appimage
                            --verbosity=3
                    RESULT_VARIABLE DEPLOY_RESULT
                )
                
                if(NOT DEPLOY_RESULT EQUAL 0)
                    message(WARNING \"linuxdeploy failed: ${DEPLOY_RESULT}\")
                endif()
            ")
        endif()
    endif()
endif()
