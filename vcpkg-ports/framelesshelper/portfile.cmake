vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO wangwenx190/framelesshelper
    REF 2.5.0
    SHA512 0612e1a4c886af7329eee43c3bda6d55f9db65d734e7432627562675cc8a0011a78bae85183fcb343d1514a8ba672df22f6cc1b0ce4ba1ef1c698356bf4ab55a
    HEAD_REF main
    PATCHES
        remove-agl-framework.patch
        add-qt-private-components.patch
        fix-quickcontrols2-private.patch
)

# 下载 cmake-utils 子模块
vcpkg_from_github(
    OUT_SOURCE_PATH CMAKE_UTILS_PATH
    REPO wangwenx190/cmake-utils
    REF main
    SHA512 029919d3680b942f2de19585627c837261a90214690d467883218ae6c92738692df5aa317b40b8610a5b164614a24b0834ccbaddd9a39c514caa4d0eda79af03
    HEAD_REF main
)

# 将 cmake-utils 内容复制到 SOURCE_PATH/cmake
file(REMOVE_RECURSE "${SOURCE_PATH}/cmake")
file(COPY "${CMAKE_UTILS_PATH}/" DESTINATION "${SOURCE_PATH}/cmake")

# 修复 cmake/utils.cmake 中已弃用的链接选项
file(READ "${SOURCE_PATH}/cmake/utils.cmake" UTILS_CMAKE_CONTENT)
string(REPLACE
    "-Wl,-fatal_warnings -Wl,-undefined,error"
    "-Wl,-undefined,dynamic_lookup"
    UTILS_CMAKE_CONTENT
    "${UTILS_CMAKE_CONTENT}")
file(WRITE "${SOURCE_PATH}/cmake/utils.cmake" "${UTILS_CMAKE_CONTENT}")

# Qt 路径通过自定义 triplet 的 VCPKG_CMAKE_CONFIGURE_OPTIONS 传递
# 请确保设置了 QTDIR 环境变量，或在 triplet 中正确配置 CMAKE_PREFIX_PATH

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DFRAMELESSHELPER_BUILD_EXAMPLES=OFF
        -DFRAMELESSHELPER_BUILD_STATIC=OFF
        -DFRAMELESSHELPER_NO_INSTALL=OFF
        -DFRAMELESSHELPER_BUILD_WIDGETS=ON
        -DFRAMELESSHELPER_BUILD_QUICK=ON
    MAYBE_UNUSED_VARIABLES
        FRAMELESSHELPER_BUILD_WIDGETS
        FRAMELESSHELPER_BUILD_QUICK
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME FramelessHelper
    CONFIG_PATH lib/cmake/FramelessHelper
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

vcpkg_copy_pdbs()
