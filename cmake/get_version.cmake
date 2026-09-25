function(get_version_info major minor build commit)
    find_package(Git REQUIRED)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --long --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT GIT_RESULT EQUAL 0 OR NOT GIT_VERSION)
        message(WARNING "Can't get Git version information, use default version")
        set(GIT_VERSION "v0.0.0-0-g0000000")
    endif()

    # git describe --tags --long --always 的三种形态:
    #   v2.2.1-0-g6c254f4          正式 tag 所在提交
    #   v2.2.1-5-g1234567          tag 之后第 5 个提交
    #   v2.3.0-beta.1-0-g9a5b2d7   预发布 tag(带 semver 预发布后缀)
    #
    # 预发布后缀必须以字母开头, 否则 describe 的提交计数("-5-g...")会被误当成后缀。
    # 后缀本身不进入版本号: MSVC 版本资源只接受 4 段数字, 所以预发布 tag 的 APP_VERSION
    # 仍是数字部分(如 v2.3.0-beta.1 -> 2.3.0); 是否算预发布由工作流按 tag 名判定。
    # 这里容忍它只是为了避免解析失败后退化成 0.0.0(那样整条发版流水线的版本号全错)。
    #
    # 捕获组: 1/2/3=主次修订, 4=带连字符的后缀, 5=带连字符的计数, 6=计数, 7=commit 短哈希
    string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[A-Za-z][0-9A-Za-z.-]*)?(-([0-9]+))?-g([0-9a-f]+)" _ ${GIT_VERSION})

    if(CMAKE_MATCH_COUNT GREATER_EQUAL 7)
        set(${major} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${minor} "${CMAKE_MATCH_2}" PARENT_SCOPE)
        set(${build} "${CMAKE_MATCH_3}" PARENT_SCOPE)
        set(${commit} "${CMAKE_MATCH_7}" PARENT_SCOPE)
    else()
        message(WARNING "The version number format does not match: ${GIT_VERSION}")
        set(${major} "0" PARENT_SCOPE)
        set(${minor} "0" PARENT_SCOPE)
        set(${build} "0" PARENT_SCOPE)
        set(${commit} "" PARENT_SCOPE)
    endif()
endfunction()
