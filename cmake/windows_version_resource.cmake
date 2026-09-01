# Windows 版本信息资源基础设施。
#
# 背景:SignPath Foundation(以及一般 Authenticode 代码签名流程)要求所有被签名的
# 二进制文件必须携带一致的 ProductName / ProductVersion 元数据,且签名请求端会通过
# artifact 配置的 file metadata restrictions 强制校验(见 SignPath Foundation
# "SignPath configuration requirements" 条款)。此前只有主程序 gdownload.exe 有版本
# 资源,自有 DLL(GDLCore/Engine/Ed2kEngine/PluginManager)缺失,导致它们无法纳入
# 签名范围。本模块补齐这一缺口。
#
# 用法(目标已创建后调用,仅 Windows 生效):
#   gdl_configure_windows_version_resources(<target>
#       [FILE_DESCRIPTION "描述文本"]
#       [ORIGINAL_FILENAME "xxx.exe|xxx.dll"])
# 依赖根 CMakeLists 定义的 CURRENT_VERSION_MAJOR/MINOR/BUILD/COMMIT、APP_VERSION
# 与 PRODUCT_DISPLAY_NAME / PRODUCT_COMPANY_NAME / PRODUCT_COPYRIGHT。
function(gdl_configure_windows_version_resources target)
    if(NOT WIN32)
        return()
    endif()
    if(NOT DEFINED CURRENT_VERSION_MAJOR)
        message(FATAL_ERROR
            "gdl_configure_windows_version_resources: CURRENT_VERSION_* 未定义,请确认 get_version_info 已先执行")
    endif()

    cmake_parse_arguments(GDLVR
        ""
        "FILE_DESCRIPTION;ORIGINAL_FILENAME;FILE_TYPE"
        ""
        ${ARGN}
    )

    # 按目标类型推断 FILETYPE 与默认 OriginalFilename,显式参数优先
    get_target_property(_gdlvr_target_type ${target} TYPE)
    if(_gdlvr_target_type STREQUAL "EXECUTABLE")
        set(GDLVR_FILE_TYPE_VALUE "VFT_APP")
        set(_gdlvr_default_ext "exe")
    else()
        set(GDLVR_FILE_TYPE_VALUE "VFT_DLL")
        set(_gdlvr_default_ext "dll")
    endif()
    if(GDLVR_FILE_TYPE)
        set(GDLVR_FILE_TYPE_VALUE "${GDLVR_FILE_TYPE}")
    endif()

    set(GDLVR_TARGET_NAME "${target}")
    set(GDLVR_PRODUCT_NAME "${PRODUCT_DISPLAY_NAME}")
    set(GDLVR_COMPANY_NAME "${PRODUCT_COMPANY_NAME}")
    if(GDLVR_FILE_DESCRIPTION)
        set(GDLVR_FILE_DESCRIPTION "${GDLVR_FILE_DESCRIPTION}")
    else()
        set(GDLVR_FILE_DESCRIPTION "${PRODUCT_DISPLAY_NAME}")
    endif()
    if(GDLVR_ORIGINAL_FILENAME)
        set(GDLVR_ORIGINAL_FILENAME "${GDLVR_ORIGINAL_FILENAME}")
    else()
        set(GDLVR_ORIGINAL_FILENAME "${target}.${_gdlvr_default_ext}")
    endif()
    set(GDLVR_COPYRIGHT "${PRODUCT_COPYRIGHT}")

    # FILEVERSION 数值四元组:commit 哈希不是数字,只能落在字符串版本里,数值位归零
    set(GDLVR_FILEVERSION_COMMA
        "${CURRENT_VERSION_MAJOR},${CURRENT_VERSION_MINOR},${CURRENT_VERSION_BUILD},0")
    set(GDLVR_PRODUCTVERSION_COMMA
        "${CURRENT_VERSION_MAJOR},${CURRENT_VERSION_MINOR},${CURRENT_VERSION_BUILD},0")
    set(GDLVR_FILEVERSION_STR
        "${CURRENT_VERSION_MAJOR}.${CURRENT_VERSION_MINOR}.${CURRENT_VERSION_BUILD}.${CURRENT_VERSION_COMMIT}")
    set(GDLVR_PRODUCTVERSION_STR "${APP_VERSION}")

    # 生成到构建目录,避免污染源码树;显式挂到目标源列表
    set(_gdlvr_rc_file "${CMAKE_CURRENT_BINARY_DIR}/version_resource/${target}_version.rc")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/WindowsVersionResource.rc.in"
        "${_gdlvr_rc_file}" @ONLY)
    target_sources(${target} PRIVATE "${_gdlvr_rc_file}")
    set_source_files_properties("${_gdlvr_rc_file}" PROPERTIES SKIP_AUTOMOC ON)
endfunction()
