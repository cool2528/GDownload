# 增加qt 的支持
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
include_directories(${MODULE_INCLUDE_PATH})
list(APPEND QT_MODULES Widgets Network Core Gui Qml Quick WebSockets Core5Compat)
set(QT_PREFIXED_MODULES ${QT_MODULES})
list(TRANSFORM QT_PREFIXED_MODULES PREPEND Qt6::)
find_package(Qt6 COMPONENTS ${QT_MODULES} REQUIRED)
# 打印 Qt6 安装路径
message(STATUS "Qt6 installation path: ${Qt6_DIR}")
message(STATUS "MODULE_INCLUDE_PATH installation path: ${MODULE_INCLUDE_PATH}")
add_subdirectory(engine)
add_subdirectory(ui)
