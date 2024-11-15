macro(INIT_SHARE_PROJECT name)
    project(${name})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/export.h)
    configure_file(${CMAKE_SOURCE_DIR}/src/export.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/export.h
    @ONLY
    )
    endif()
endmacro()
