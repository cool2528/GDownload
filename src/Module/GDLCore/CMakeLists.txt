
project(GDLCore LANGUAGES CXX C)

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

find_package(spdlog CONFIG REQUIRED)
add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
