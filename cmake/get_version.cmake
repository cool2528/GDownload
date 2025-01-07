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
        set(GIT_VERSION "0.0-0-g0000000")
    endif()

    string(REGEX MATCH "^v?([0-9]+)\\.([0-9]+)(-([0-9]+))?-g([0-9a-f]+)" _ ${GIT_VERSION})

    if(CMAKE_MATCH_COUNT GREATER_EQUAL 5)
        set(${major} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${minor} "${CMAKE_MATCH_2}" PARENT_SCOPE)
        if(CMAKE_MATCH_4)
            set(${build} "${CMAKE_MATCH_4}" PARENT_SCOPE)
        else()
            set(${build} "0" PARENT_SCOPE)
        endif()
        set(${commit} "${CMAKE_MATCH_5}" PARENT_SCOPE)
    else()
        message(WARNING "The version number format does not match: ${GIT_VERSION}")
        set(${major} "0" PARENT_SCOPE)
        set(${minor} "0" PARENT_SCOPE)
        set(${build} "0" PARENT_SCOPE)
        set(${commit} "" PARENT_SCOPE)
    endif()
endfunction()
