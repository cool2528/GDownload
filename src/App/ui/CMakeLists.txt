cmake_minimum_required(VERSION 3.10)

project(gdownload LANGUAGES CXX C)

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

file(GLOB_RECURSE QT_RESOURCE_FILES  
"${CMAKE_SOURCE_DIR}/resource/*.qrc"
)
file(GLOB_RECURSE QT_QML_FILES 
"${CMAKE_SOURCE_DIR}/qml/*.qml"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "header" FILES ${HEADERS_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "resource" FILES ${QT_RESOURCE_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "qml" FILES ${QT_QML_FILES})

add_executable(${PROJECT_NAME} ${SOURCES_FILES} ${HEADERS_FILES})



