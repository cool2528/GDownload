cmake_minimum_required(VERSION 3.18)
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
# 设置依赖库的根路径
if(APPLE)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/osx")
elseif(WIN32)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/windows")
elseif(UNIX AND NOT APPLE)
    set(DEPENDENCIES_ROOT_PATH "${CMAKE_SOURCE_DIR}/dependencies/linux")
endif()
project(GDownload LANGUAGES CXX C)

add_subdirectory(src)

# 安装规则
if(WIN32)
    # Windows 安装规则
    install(TARGETS gdownload
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
    )
    install(FILES README.md LICENSE DESTINATION .)
    # 设置默认安装前缀为程序文件目录
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/GDownload" CACHE PATH "Installation prefix" FORCE)
    endif()
elseif(APPLE)
    # macOS 安装规则
    install(TARGETS gdownload
        BUNDLE DESTINATION ${CMAKE_BINARY_DIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(FILES README.md LICENSE DESTINATION Resources)
    # 设置默认安装前缀为应用程序目录
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/Applications" CACHE PATH "Installation prefix" FORCE)
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux 安装规则
    install(TARGETS gdownload
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(FILES README.md LICENSE DESTINATION share/GDownload)
    # 设置默认安装前缀为 /usr/local
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix" FORCE)
    endif()
endif()

