set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_BUILD_TYPE release)

# 从环境变量获取 Qt 路径并添加到 CMake 搜索路径
# 这样 vcpkg 构建的包（如 FramelessHelper）也能找到 Qt
if(DEFINED ENV{QTDIR})
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_PREFIX_PATH=$ENV{QTDIR}")
    message(STATUS "[Custom Triplet] Qt path set to: $ENV{QTDIR}")
else()
    message(WARNING "[Custom Triplet] QTDIR environment variable not set. Qt-dependent packages may fail to build.")
endif()