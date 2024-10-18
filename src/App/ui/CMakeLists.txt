

project(gdownload LANGUAGES CXX C)

file(GLOB_RECURSE SOURCES_FILES
    *.cpp
    *.cxx
    *.c
)
file(GLOB_RECURSE HEADERS_FILES
    *.h
    *.hpp
    *.hxx
)

file(GLOB_RECURSE QT_RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/*.qrc"
)
file(GLOB_RECURSE QT_QML_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource/qml/*.qml"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "resource" FILES ${QT_RESOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "qml" FILES ${QT_QML_FILES})

# 输出CMAKE_BINARY_DIR 的路径
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

# set FramelessHelper_DIR
set(FramelessHelper_DIR ${DEPENDENCIES_ROOT_PATH}/FramelessHelper/lib/cmake)
list(APPEND CMAKE_PREFIX_PATH ${DEPENDENCIES_ROOT_PATH}/FramelessHelper)
set(FRAMELESSHELPER_IMPORT_DIR ${DEPENDENCIES_ROOT_PATH}/FramelessHelper/qml)
find_package(FramelessHelper REQUIRED COMPONENTS Core Widgets Quick)
list(APPEND QML_IMPORT_PATH "${FRAMELESSHELPER_IMPORT_DIR}")
list(REMOVE_DUPLICATES QML_IMPORT_PATH)
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} CACHE STRING "Qt Creator extra QML import paths" FORCE)

# set dependencies library
set(DEPENDENCIES_LIBRARIES FramelessHelper::Core FramelessHelper::Widgets FramelessHelper::Quick GDLCore)
list(APPEND DEPENDENCIES_LIBRARIES ${QT_PREFIXED_MODULES})

if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES_FILES} ${HEADERS_FILES} ${QT_RESOURCE_FILES} ${QT_QML_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/GDownloadOsxBundleInfo.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.gdownload.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_ICON_FILE "logo.icns"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    )

    # 复制logo.icns到捆绑包的资源目录中
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Resource/images/logo/logo.icns
        ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/${PROJECT_NAME}.app/Contents/Resources/logo.icns
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} ${SOURCES_FILES} ${HEADERS_FILES} ${QT_RESOURCE_FILES} ${QT_QML_FILES})
endif()
target_include_directories(${PROJECT_NAME} PRIVATE ${MODULE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCIES_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE
   $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)
