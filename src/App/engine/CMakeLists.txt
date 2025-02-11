
INIT_SHARE_PROJECT(Engine)

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
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES})
find_package(boost_system CONFIG REQUIRED)
find_package(boost_property_tree CONFIG REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_url CONFIG REQUIRED)
find_package(boost_beast CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(SECURITY_FRAMEWORK Security)
endif()
if(MSVC)
add_compile_options(/bigobj)
endif()
add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${MODULE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system
Boost::property_tree
Boost::asio 
nlohmann_json::nlohmann_json 
Boost::url 
cpr::cpr 
Boost::beast 
GDLCore 
PluginManager
rapidjson
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
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES
FOLDER "App"
)
