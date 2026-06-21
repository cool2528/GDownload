# 把 manifest.jsonl 折叠为 manifest.json(JSON 数组)
# 测试期间各用例 append 一行 JSON 到 manifest.jsonl,
# 全部测试结束后由 fixture CLEANUP 调用本脚本折叠成数组写回 manifest.json
#
# 注意:本脚本由 ctest 以 `cmake -P` 方式调用,脚本模式下 CMAKE_BINARY_DIR 未定义。
# 通过 set_tests_properties(... ENVIRONMENT ...) 注入 QML_UI_ARTIFACT_DIR 环境变量
# 拿到产物目录的绝对路径(由 tests/qml_ui/CMakeLists.txt 设置)。
if(NOT DEFINED ENV{QML_UI_ARTIFACT_DIR})
    message(WARNING "QML_UI_ARTIFACT_DIR not set, skip manifest finalization")
    return()
endif()
set(ARTIFACT_DIR "$ENV{QML_UI_ARTIFACT_DIR}")
if(EXISTS "${ARTIFACT_DIR}/manifest.jsonl")
    file(READ "${ARTIFACT_DIR}/manifest.jsonl" LINES)
    # 行尾换行替换为逗号换行,形成 JSONL -> JSON 数组元素分隔
    string(REPLACE "\n" ",\n" LINES "${LINES}")
    # 去掉末尾多余逗号(最后一行后的逗号)
    string(REGEX REPLACE ",\n$" "\n" LINES "${LINES}")
    file(WRITE "${ARTIFACT_DIR}/manifest.json"
         "[\n${LINES}]\n")
endif()
