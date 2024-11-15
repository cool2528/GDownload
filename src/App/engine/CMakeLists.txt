
project(engine LANGUAGES CXX C)

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
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(boost_url CONFIG REQUIRED)
add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${MODULE_INCLUDE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::property_tree)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::asio)
target_link_libraries(${PROJECT_NAME} PRIVATE httplib::httplib)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::url)
target_link_libraries(${PROJECT_NAME} PRIVATE GDLCore)
