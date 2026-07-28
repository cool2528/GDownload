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
    # 这里原本有一条"不要定义 UNICODE/_UNICODE"的禁令：引擎公开头里曾有同名常量
    # ed2k::server::srvflag::UNICODE，会被预处理器宏原样替换成语法错误。
    # 引擎已把那两个常量改名为 srvflag::UTF8 / udpflag::UTF8（eMule 对 UDP 侧同一能力位就叫
    # SRV_UDPFLG_SUPPORTUTF8），禁令随之解除 —— 本目标现在可以正常定义这两个宏。
    # 之所以仍然不定义：本库不直接调用 Win32 TCHAR 系 API，加了没有收益。
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        Ed2kEngine_EXPORTS
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
FOLDER "App"
)
