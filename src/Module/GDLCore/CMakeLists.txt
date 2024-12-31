if(APPLE)
    project(GDLCore LANGUAGES CXX C OBJCXX)
else()
    project(GDLCore LANGUAGES CXX C)
endif()

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
find_package(spdlog CONFIG REQUIRED)
find_package(boost_property_tree CONFIG REQUIRED)
find_package(boost_beast CONFIG REQUIRED)
find_package(boost_locale CONFIG REQUIRED)
if(APPLE)
    file(GLOB_RECURSE SOURCE_OBJ_FILES
        *.mm
    )
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(FOUNDATION_FRAMEWORK Foundation)
      find_library(CFNETWORK_FRAMEWORK CFNetwork)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES} ${SOURCE_OBJ_FILES})
    add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES} ${SOURCE_OBJ_FILES})
else()
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "source" FILES ${SOURCES_FILES} ${HEADERS_FILES})
    add_library(${PROJECT_NAME} SHARED ${SOURCES_FILES} ${HEADERS_FILES})
endif()


register_target(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog Boost::property_tree Boost::beast Boost::locale)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COREFOUNDATION_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${CFNETWORK_FRAMEWORK}
    )
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
    UNICODE 
    _UNICODE
    )
endif()
