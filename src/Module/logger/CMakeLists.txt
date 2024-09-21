cmake_minimum_required(VERSION 3.10)

project(logger LANGUAGES CXX C)

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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "header" FILES ${HEADERS_FILES})

add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
