set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# 最低支持 macOS 11.0，避免在 macOS 26 SDK 上默认取 26.0 导致旧系统无法启动 (error -10825)
set(VCPKG_OSX_DEPLOYMENT_TARGET "11.0")

# 允许 QTDIR 环境变量传递到 vcpkg 构建环境
set(VCPKG_ENV_PASSTHROUGH QTDIR)

# 从环境变量获取 Qt 路径并添加到 CMake 搜索路径
# 这样 vcpkg 构建的包（如 FramelessHelper）也能找到 Qt
if(DEFINED ENV{QTDIR})
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_PREFIX_PATH=$ENV{QTDIR}")
    message(STATUS "[Custom Triplet] Qt path set to: $ENV{QTDIR}")
else()
    message(WARNING "[Custom Triplet] QTDIR environment variable not set. Qt-dependent packages may fail to build.")
endif()
