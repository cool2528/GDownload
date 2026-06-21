# 把 manifest.jsonl 折叠为 manifest.json(JSON 数组)
# 测试期间各用例 append 一行 JSON 到 manifest.jsonl,
# 全部测试结束后由 fixture CLEANUP 调用本脚本折叠成数组写回 manifest.json
if(EXISTS "${CMAKE_BINARY_DIR}/test_artifacts/qml_ui/manifest.jsonl")
    file(READ "${CMAKE_BINARY_DIR}/test_artifacts/qml_ui/manifest.jsonl" LINES)
    # 行尾换行替换为逗号换行,形成 JSONL -> JSON 数组元素分隔
    string(REPLACE "\n" ",\n" LINES "${LINES}")
    # 去掉末尾多余逗号(最后一行后的逗号)
    string(REGEX REPLACE ",\n$" "\n" LINES "${LINES}")
    file(WRITE "${CMAKE_BINARY_DIR}/test_artifacts/qml_ui/manifest.json"
         "[\n${LINES}]\n")
endif()
