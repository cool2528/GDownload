vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cool2528/ed2k-engine-cpp
    REF v${VERSION}
    SHA512 dfcca49e8728b20e5dcb854896e151b677f446fcbcdd2f7bc3a22a304fc9f58ccdf13e69cdced8736b14014499a8824476a7b96ffda842ebf0c4eaf725a00014
    HEAD_REF main
)

# 引擎大部分类未标导出宏(ED2K_EXPORT)，其自身 CLI/测试一直静态链接 ed2k_core。
# 在动态 triplet 下 vcpkg 会把 ed2k_core 建成 DLL，导致捆绑的 ed2k-tool 链接时
# 找不到未导出的 ed2k::* 符号(LNK2019)。强制静态链接即可(与 quickjs-ng port 同做法)，
# 消费方 GDownload 也静态链入 ed2k::core，无需分发 ed2k_core.dll。
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DED2K_BUILD_TESTS=OFF
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ed2k PACKAGE_NAME ed2k)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
