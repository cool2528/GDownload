vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cool2528/ed2k-engine-cpp
    REF v${VERSION}
    SHA512 c90136f846b3228403f60330adca7fcbc262948ab30675d89b944a826da3d1715ac3eff93c77daeef23fffa0b483c599ec9480bfc8dfdfc71565101de0f2c176
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

# 上游 install() 把 ed2k-tool 装到 bin/，但导出的 ed2k::ed2k-tool 目标要求它位于
# tools/ed2k-engine/ 下（vcpkg 工具二进制的标准位置）。用 vcpkg_copy_tools 挪过去，
# AUTO_CLEAN 顺带清掉 bin/ 下的原始副本，避免 vcpkg 的重复文件策略检查报错。
vcpkg_copy_tools(TOOL_NAMES ed2k-tool AUTO_CLEAN)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
