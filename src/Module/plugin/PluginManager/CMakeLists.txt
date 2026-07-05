INIT_SHARE_PROJECT(PluginManager)
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
find_package(boost_system CONFIG REQUIRED)
find_package(boost_property_tree CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_url CONFIG REQUIRED)
if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(SECURITY_FRAMEWORK Security)
endif()
add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${MODULE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        GDLCore
        Boost::system 
        Boost::property_tree 
        Boost::asio 
        cpr::cpr 
        nlohmann_json::nlohmann_json 
        Boost::url
        OpenSSL::Crypto
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${SECURITY_FRAMEWORK}
    )
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        UNICODE 
        _UNICODE
    )
elseif(UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PluginManager_EXPORTS
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
FOLDER "Plugin"
)

add_dependencies(${PROJECT_NAME}
    GDLCore
    Baidu_Plugin
)
