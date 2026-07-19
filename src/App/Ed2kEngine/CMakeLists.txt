INIT_SHARE_PROJECT(Ed2kEngine)

file(GLOB_RECURSE SOURCES_FILES CONFIGURE_DEPENDS
*.cpp
*.cxx
*.c
)
file(GLOB_RECURSE HEADERS_FILES CONFIGURE_DEPENDS
*.h
*.hpp
*.hxx
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES})
find_package(ed2k CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${MODULE_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/App/engine)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ed2k::core
    Boost::asio
    nlohmann_json::nlohmann_json
    GDLCore
)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Advapi32)
    # 注意：不要定义 UNICODE/_UNICODE —— ed2k 引擎头文件里有同名常量
    # ed2k::server::srvflag::UNICODE，会被预处理器宏原样替换导致语法错误。
    # 本库当前不直接调用 Win32 TCHAR 系 API，无需这两个宏。
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        Ed2kEngine_EXPORTS
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
FOLDER "App"
)
