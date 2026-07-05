if(APPLE)
    project(gdownload LANGUAGES CXX C OBJCXX)
    file(GLOB_RECURSE SOURCES_FILES CONFIGURE_DEPENDS
    *.cpp
    *.cxx
    *.c
    *.mm
    )
    foreach(file ${SOURCES_FILES})
    if(file MATCHES "\\.mm$")
        set_property(SOURCE ${file} PROPERTY SKIP_AUTOMOC ON)
    endif()
    endforeach()

    set(SPARKLE_FRAMEWORK_PATH "${SPARKLE_ROOT_PATH}/Sparkle.framework")
    if(EXISTS ${SPARKLE_FRAMEWORK_PATH})
        message("SPARKLE_FRAMEWORK_PATH: ${SPARKLE_FRAMEWORK_PATH}")
        set(SPARKLE_FRAMEWORK_PATH ${SPARKLE_FRAMEWORK_PATH} CACHE PATH "Sparkle framework path")
    endif()
    set(SPARKLE_INCLUDE_PATH "${SPARKLE_FRAMEWORK_PATH}/Headers")
    if(EXISTS ${SPARKLE_INCLUDE_PATH})
        message("SPARKLE_INCLUDE_PATH: ${SPARKLE_INCLUDE_PATH}")
        set(SPARKLE_INCLUDE_PATH ${SPARKLE_INCLUDE_PATH} CACHE PATH "Sparkle include path")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -F${SPARKLE_ROOT_PATH}")
else()
    project(gdownload LANGUAGES CXX C)
    file(GLOB_RECURSE SOURCES_FILES CONFIGURE_DEPENDS
    *.cpp
    *.cxx
    *.c
    )
   
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/WindowsResourceTemplate.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc) 
endif()
file(GLOB_RECURSE HEADERS_FILES CONFIGURE_DEPENDS
    *.h
    *.hpp
    *.hxx
)

file(GLOB_RECURSE QT_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/*.qrc"
)
file(GLOB_RECURSE QT_QML_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/qml/*.qml"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "resource" FILES ${QT_RESOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "qml" FILES ${QT_QML_FILES})

if(GDL_UI_GENERATE_TRANSLATIONS)
    find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
endif()
# 生成翻译ts文件
set(QT_TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/gdownload_ja_JP.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/gdownload_zh_CN.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/gdownload_zh_TW.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/gdownload_ko_KR.ts"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "ts" FILES ${QT_TS_FILES})
set(QT_ROOT "${Qt6_DIR}/../../..")
# 使用foreach遍历所有ts文件
if(GDL_UI_GENERATE_TRANSLATIONS)
    if(WIN32)
        message("update ts file for windows QT_ROOT: ${QT_ROOT}")
        foreach(TS_FILE ${QT_TS_FILES})
            message("update ts file: ${TS_FILE}")
            execute_process(
                COMMAND ${QT_ROOT}/bin/lupdate.exe 
                ${CMAKE_CURRENT_SOURCE_DIR}
                -ts ${TS_FILE}
                -no-obsolete
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            # 获取不带后缀的文件名
            get_filename_component(QM_FILE ${TS_FILE} NAME_WE)
            # 根据ts文件生成qm文件
            execute_process(
                COMMAND ${QT_ROOT}/bin/lrelease
                ${TS_FILE}
                -qm ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/${QM_FILE}.qm
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations
            )
        endforeach()
    else()
        message("update ts file for linux QT_ROOT: ${QT_ROOT}")
        foreach(TS_FILE ${QT_TS_FILES})
            message("update ts file: ${TS_FILE}")
            execute_process(
                COMMAND ${QT_ROOT}/bin/lupdate
                ${CMAKE_CURRENT_SOURCE_DIR}
                -ts ${TS_FILE}
                -no-obsolete
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            # 获取不带后缀的文件名
            get_filename_component(QM_FILE ${TS_FILE} NAME_WE)
            # 根据ts文件生成qm文件
            execute_process(
                COMMAND ${QT_ROOT}/bin/lrelease
                ${TS_FILE}
                -qm ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations/${QM_FILE}.qm
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations
            )
        endforeach()
    endif()
else()
    message(STATUS "Skipping translation update (GDL_UI_GENERATE_TRANSLATIONS=OFF)")
endif()

# 如果 resource/docs 不存在notice 文件则复制到这个目录下
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resource/docs/NOTICE)
    file(COPY ${CMAKE_SOURCE_DIR}/NOTICE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Resource/docs)
endif()

# 输出CMAKE_BINARY_DIR 的路径
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

find_package(FramelessHelper REQUIRED COMPONENTS Core Widgets Quick)
find_package(boost_system CONFIG REQUIRED)
find_package(boost_property_tree CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

# set dependencies library
set(DEPENDENCIES_LIBRARIES FramelessHelper::Core FramelessHelper::Widgets FramelessHelper::Quick GDLCore Engine PluginManager)
list(APPEND DEPENDENCIES_LIBRARIES ${QT_PREFIXED_MODULES})
# 打印操作系统名称
message("Operating System: ${CMAKE_SYSTEM_NAME}")

# 打印系统架构
message("System Architecture: ${CMAKE_SYSTEM_PROCESSOR}")

if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES_FILES} ${HEADERS_FILES} ${QT_RESOURCE_FILES} ${QT_QML_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/GDownloadOsxBundleInfo.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gdownload.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${CURRENT_VERSION_MAJOR}.${CURRENT_VERSION_MINOR}.${CURRENT_VERSION_BUILD}"
        MACOSX_BUNDLE_ICON_FILE "logo.icns"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
        BUNDLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin/${PROJECT_NAME}.app/Contents/Frameworks/Sparkle.framework
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${SPARKLE_ROOT_PATH}/Sparkle.framework
                ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin/${PROJECT_NAME}.app/Contents/Frameworks/Sparkle.framework
        )
    endif()
    # 复制logo.icns到捆绑包的资源目录中
    # Determine the correct engine directory based on target architecture
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64;x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "x86_64;arm64")
        set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/extra/darwin/universal/engine/")
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/extra/darwin/arm64/engine/")
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/extra/darwin/x86_64/engine/")
    else()
        # Fallback to CMAKE_SYSTEM_PROCESSOR (may be incorrect for cross-compilation)
        set(ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/extra/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/engine/")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Resource/images/logo/logo.icns
        ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/Resources/logo.icns
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/Resources/engine
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ENGINE_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/Resources/engine
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations
            ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/Resources/translations
        COMMAND $<$<CONFIG:Debug>:install_name_tool>
            $<$<CONFIG:Debug>:-add_rpath>
            $<$<CONFIG:Debug>:${QT_ROOT}/lib>
            $<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}>
        VERBATIM
    )
elseif(WIN32)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EXECUTABLE ${PROJECT_NAME}.exe)
    configure_file(
        ${CMAKE_SOURCE_DIR}/package/windows_installer.iss.in
        ${CMAKE_BINARY_DIR}/release/windows_installer.iss
        @ONLY
    )
    endif()
    # 检查生成器类型
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(OUTPUT_BIN_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin/${CMAKE_BUILD_TYPE}")
    else()
    # Ninja 或其他生成器
    set(OUTPUT_BIN_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin")
    endif()
    # 打印生成器信息
    message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
    message(STATUS "OUTPUT_BIN_PATH: ${OUTPUT_BIN_PATH}")
    list(APPEND QT_RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
    endif()
    add_executable(${PROJECT_NAME} ${SOURCES_FILES} ${HEADERS_FILES} ${QT_RESOURCE_FILES} ${QT_QML_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    add_custom_command(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
    # 增加调用qt的 windeployqt
    add_custom_command(TARGET ${PROJECT_NAME} 
        POST_BUILD
        COMMAND ${QT_ROOT}/bin/windeployqt.exe $<TARGET_FILE:${PROJECT_NAME}>
        --qmlimport ${QT_ROOT}/qml
        --qmldir ${CMAKE_CURRENT_SOURCE_DIR}/Resource/qml
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${OUTPUT_BIN_PATH}/engine
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/extra/win32/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,ia32>/engine/
        ${OUTPUT_BIN_PATH}/engine
        # 复制qm文件
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations
        ${OUTPUT_BIN_PATH}/translations
    )
elseif(UNIX AND NOT APPLE)
    add_executable(${PROJECT_NAME} ${SOURCES_FILES} ${HEADERS_FILES} ${QT_RESOURCE_FILES} ${QT_QML_FILES})
    message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    add_custom_command(TARGET ${PROJECT_NAME} 
    POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/extra/linux/$<IF:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>,x64,arm64>/engine/
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/engine
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Resource/translations
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/translations
        COMMAND_EXPAND_LISTS
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE libappimageupdate)
endif()
register_target(${PROJECT_NAME})



target_include_directories(${PROJECT_NAME} PRIVATE ${MODULE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/App/engine
    $<$<CXX_COMPILER_ID:AppleClang>:${SPARKLE_FRAMEWORK_PATH}>
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCIES_LIBRARIES} 
$<$<CXX_COMPILER_ID:AppleClang>:${SPARKLE_ROOT_PATH}>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
   $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::property_tree)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::asio)
if(APPLE)
target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Sparkle")
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE LibtorrentRasterbar::torrent-rasterbar pugixml::shared pugixml::pugixml)
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        UNICODE 
        _UNICODE
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
FOLDER "App"
)




