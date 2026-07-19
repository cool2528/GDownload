cmake_minimum_required(VERSION 3.18)
project(gdownload_native_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Native Messaging host：轻量控制台程序，仅依赖 nlohmann-json + toml++（均 header-only）
# 刻意不链接 GDLCore，避免引入 boost/sqlite/openssl 等重型依赖
find_package(nlohmann_json CONFIG REQUIRED)

find_path(TOMLPLUSPLUS_INCLUDE_DIR
    NAMES "toml++/toml.h"
    HINTS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES include
)
if(NOT TOMLPLUSPLUS_INCLUDE_DIR)
    message(FATAL_ERROR "Failed to locate toml++ headers. Ensure tomlplusplus is installed (e.g. via vcpkg).")
endif()

add_executable(gdownload_native_host main.cpp)
target_include_directories(gdownload_native_host PRIVATE ${TOMLPLUSPLUS_INCLUDE_DIR})
target_link_libraries(gdownload_native_host PRIVATE nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(gdownload_native_host PRIVATE ws2_32 Shell32)
    target_compile_definitions(gdownload_native_host PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# MSVC 下按 UTF-8 解析源文件，避免中文注释在 GBK locale 被误判续行
if(MSVC)
    target_compile_options(gdownload_native_host PRIVATE /utf-8)
endif()

set_target_properties(gdownload_native_host PROPERTIES FOLDER "Tools")
