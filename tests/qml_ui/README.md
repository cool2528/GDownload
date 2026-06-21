# QML UI 测试设施

本目录承载 GDownload 的 QML UI 测试,分两类:

- `visual/` — Qt Quick Test(`.qml`)视觉验证用例,截图产物落盘
- `integration/` — QtTest C++ 集成用例,模拟用户操作断言后端行为
- `support/` — 共享设施:`FakeBrowserManager` / `FakeSettingsManager` 替身、`ScreenshotHelper`、`PageHarness.qml`

## 快速起测

```bash
# 配置(首次或 CMakeLists 变更时)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=$QTDIR

# 构建
cmake --build build --config Debug --parallel

# 跑全部 qml_ui 测试
cmake --build build --target run_qml_ui_tests
# 或直接
cd build && ctest -L qml_ui --output-on-failure
```

## 产物

- 截图:`build/test_artifacts/qml_ui/<test>/<case>_<theme>.png`
- 清单:`build/test_artifacts/qml_ui/manifest.jsonl`(测试期)→ `manifest.json`(fixture cleanup 折叠)

## 环境变量

- `GDOWNLOAD_TEST=1` — 启用测试模式(跳过 aria2c 子进程 / 网络)
- `QML_UI_ARTIFACT_DIR` — 截图产物目录(默认 `build/test_artifacts/qml_ui`)

## Phase 进度

- Phase 1(Task 1-3):Manager 接口抽象 + 工厂 + 测试模式开关 — 已完成
- Phase 2(Task 4):qml_ui 骨架 + smoke — 本提交
- Phase 3(Task 5-8):27 视觉用例 × 2 主题 — 待办
- Phase 4(Task 9-12):5 集成用例 + objectName — 待办
- Phase 5(Task 13-14):README 完整 + memory 更新 — 待办

> 本 README 为 Phase 2 占位,完整版(如何加新用例 / 如何看截图 / P2 待办)在 Task 13 补全。
