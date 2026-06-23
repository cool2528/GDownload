# QML UI 测试设施

GDownload 的 QML UI 测试设施,覆盖视觉验证与集成验证两类用例,统称为 `qml_ui` 测试集。设计文档:`docs/superpowers/specs/2026-06-20-qml-ui-test-harness-design.md`,实施计划:`docs/superpowers/plans/2026-06-20-qml-ui-test-harness.md`。

- `visual/` — Qt Quick Test(`.qml`)视觉用例,offscreen 平台截图落盘
- `integration/` — QtTest C++ 集成用例,加载真实 QML 页面 + Fake 后端,模拟用户操作断言后端行为
- `support/` — 共享设施:`FakeBrowserManager` / `FakeSettingsManager` 替身、`ScreenshotHelper`、`PageHarness.qml`、`IntegrationHelper`

## 1. 快速起测

```bash
# 配置(首次或 CMakeLists 变更时)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=$QTDIR

# 构建
cmake --build build --config Debug --parallel

# 跑全部 qml_ui 测试(便利目标)
cmake --build build --target run_qml_ui_tests
# 或直接调 ctest(多配置生成器必须带 -C Debug)
cd build && ctest -C Debug -L qml_ui --output-on-failure
```

环境变量:

- `GDOWNLOAD_TEST=1` — 启用测试模式,启动跳过 aria2c 子进程 / tracker / auto-update / plugins(集成用例在 `initTestCase` 里 `qputenv`)
- `QML_UI_ARTIFACT_DIR` — 截图产物目录(默认 `build/test_artifacts/qml_ui`,由 `finalize_manifest` fixture 读取)
- `BUILD_TESTS` — CMake 选项,默认 `OFF`,开启后才注册 `qml_ui` 测试

## 2. 目录结构

```
tests/qml_ui/
├── CMakeLists.txt          # 注册 fixture SETUP/CLEANUP + 便利目标 run_qml_ui_tests
├── finalize_manifest.cmake # fixture CLEANUP: manifest.jsonl → manifest.json (cmake -P 脚本)
├── README.md               # 本文件
├── support/                # 共享设施(链接进 visual 与 integration 两个 exe)
│   ├── ScreenshotHelper.{h,cxx}   # QML 注册名 Screenshot,capture/captureWindow
│   ├── PageHarness.qml            # 测试页装载器(Loader + themeMode + offscreen 守卫)
│   ├── FakeBrowserManager.{h,cxx} # 镜像 IBrowserManager 接口的独立 QObject
│   ├── FakeSettingsManager.{h,cxx}# 镜像 ISettings 接口 + FAKE_SETTING 宏
│   ├── TestStubs.{h,cxx}          # GTheme 桩(用于 tst_theme_toggle)
│   ├── IntegrationHelper.{h,cxx}  # setupIntegrationEngine(): qmlRegisterSingletonInstance<Fake*>
│   └── GMessageBoxMulti.qml       # 测试用多按钮 GMessageBox(供 dialog 视觉用例)
├── visual/                 # Qt Quick Test 视觉用例(单 exe: qml_ui_visual)
│   ├── main.cpp            # 加载 qtbookrunner,扫描 tst_*.qml
│   ├── tst_main_views.qml       # 10 张:mainwindow/full_window/navigator/download/netdisk × 2 主题
│   ├── tst_settings_pages.qml   # 28 张:14 设置页 × 2 主题
│   ├── tst_lab_subcards.qml     # 8 张:4 Lab 子卡片 × 2 主题
│   ├── tst_dialogs.qml          # 10 张:5 对话框 × 2 主题
│   ├── tst_design_system_pages.qml # Design-system screenshots: task dialog, NetDisk workflow, download empty state, feedback
│   ├── tst_screenshot_helper.qml # smoke:验证 ScreenshotHelper 自身能抓图
│   └── tst_smoke.qml            # smoke:验证 qml_ui_visual exe 起得来
└── integration/            # QtTest C++ 集成用例(每用例独立 exe)
    ├── tst_smoke.cpp              # smoke:验证 Fake* 能注册到 QML
    ├── tst_save_settings.cpp      # 设置页 Save 写入 / Cancel 不写入
    ├── tst_create_task.cpp        # 创建任务对话框输入 → Aria2c 提交
    ├── tst_close_confirm.cpp      # 关闭确认对话框按钮交互
    ├── tst_theme_toggle.cpp       # 主题 token 切换(TestGTheme 桩)
    └── tst_navigation.cpp         # 侧栏导航 objectName 路由
```

## 3. 如何看截图

截图落盘目录:`build/test_artifacts/qml_ui/`

```
build/test_artifacts/qml_ui/
├── manifest.jsonl          # 测试期逐条 append(每行一个 JSON 对象)
├── manifest.json           # fixture CLEANUP 折叠成的 JSON 数组(最终产物)
├── tst_main_views/         # 10 张 PNG
├── tst_settings_pages/     # 28 张 PNG
├── tst_lab_subcards/       # 8 张 PNG
├── tst_dialogs/            # 10 张 PNG
├── tst_design_system_pages/ # Design-system page family screenshots
└── tst_screenshot_helper/  # smoke 1 张 PNG
```

`manifest.json` 字段(JSON 数组,每元素一行):

| 字段   | 含义                                    | 示例                                    |
|--------|-----------------------------------------|-----------------------------------------|
| `test` | 视觉用例文件名(不含扩展名)            | `tst_dialogs`                           |
| `case` | `test fn name` 派生的用例标识           | `closeconfirm_light`                    |
| `page` | 与 case 同义(历史命名)                | `closeconfirm_light`                    |
| `theme`| 截图时主题                              | `light` / `dark`                        |
| `file` | PNG 相对路径(相对 artifact 目录)     | `tst_dialogs/closeconfirm_light.png`    |
| `md5`  | PNG 字节 md5(用于像素 diff 基线对比) | `f5e2d4403d259da003e2b28b76915df3`       |
| `ts`   | ISO8601 截图时间戳(UTC)                | `2026-06-21T11:28:20Z`                  |

快速查询:

```bash
python -c "import json; m=json.load(open('build/test_artifacts/qml_ui/manifest.json')); print(len(m), 'entries')"
# 按用例分组统计
python -c "import json,collections; m=json.load(open('build/test_artifacts/qml_ui/manifest.json')); print(dict(collections.Counter(e['test'] for e in m)))"
```

## 4. 如何加新视觉用例

1. 选一个 `visual/tst_*.qml`(已有 4 个视觉用例 + 2 个 smoke),或新建 `tst_<场景>.qml`。
2. 在文件里加 `TestCase { name: "<case>"; function test_<tag>() { ... } }`。
3. 通过 `PageHarness` 装载被测页面,调 `Screenshot.capture(item, tag, harness.themeMode)` 抓图:

```qml
import QtQuick
import QtTest
import "qrc:/tests/qml_ui/support"

TestCase {
    name: "my_page"
    when: windowShown

    function test_my_page_light() {
        var harness = createHarnessWithPage("qrc:/qml/Browser/MyPage.qml", "light")
        wait(200)  // 等 Loader 异步加载完成
        verify(harness.isReady)
        var ok = Screenshot.capture(harness.pageItem, "my_page_light", harness.themeMode)
        verify(ok, "capture failed")
    }
}
```

4. `Screenshot` API:
   - `capture(item, tag, theme)` — 抓 `item` 自身(grabToImage)
   - `captureWindow(item, tag, theme)` — 抓 `item` 所在 window 的 contentItem(含标题栏装饰,用于 popup 型对话框)
   - `theme` 省略时从 `harness.themeMode` 透传;文件名格式 `<tag>.png`,落 `<test 目录>/<tag>.png`,自动 append manifest.jsonl
5. 新建 qml 文件需在 `visual/CMakeLists.txt` 的 qrc 列表里加一项;新建用例无需改 CMake(main.cpp 用 qtbookrunner 扫描)。

## 5. 如何加新集成用例

1. 新建 `integration/tst_<场景>.cpp`,继承 `QObject` + `Q_OBJECT` + `private slots:` 写 test 函数。
2. 在 `initTestCase` 里:`qputenv("GDOWNLOAD_TEST", "1")` + `setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_)`。
3. 用 `QQmlComponent` 加载被测页面,`findChild<QObject*>("objectName")` 找控件,`setProperty` / `QMetaObject::invokeMethod` 模拟用户操作,断言 `fakeSettings_->writeHistory` / `fakeBrowser_->*History`。
4. 在 `integration/CMakeLists.txt` 复制 `tst_save_settings` 的 `add_qml_ui_integration_test(tst_<场景>)` 调用注册新 exe。
5. Fake 替身用法见 `support/FakeBrowserManager.h` / `FakeSettingsManager.h`:它们是独立 `QObject`(不继承 Impl,见已知限制),镜像 `IBrowserManager` / `ISettings` 接口,通过 `FAKE_SETTING(PropName, Type, default)` 宏批量声明 getter/setter + `writeHistory` 记录。

模板(精简自 `tst_save_settings.cpp`):

```cpp
class TstMyScenario : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase() {
        qputenv("GDOWNLOAD_TEST", "1");
        fakeBrowser_ = new FakeBrowserManager(this);
        fakeSettings_ = new FakeSettingsManager(this);
        setupIntegrationEngine(&engine_, fakeBrowser_, fakeSettings_);
    }
    void test_my_flow() {
        QQmlComponent comp(&engine_, QUrl("qrc:/qml/Browser/MyPage.qml"));
        QVERIFY2(!comp.isError(), qPrintable(comp.errorString()));
        QScopedPointer<QObject> page(comp.create());
        auto* btn = page->findChild<QObject*>("btnDoThing");
        QVERIFY(btn);
        QMetaObject::invokeMethod(btn, "clicked");
        QCOMPARE(fakeBrowser_->createTaskCalls().size(), 1);
    }
   private:
    QQmlEngine engine_;
    FakeBrowserManager* fakeBrowser_ = nullptr;
    FakeSettingsManager* fakeSettings_ = nullptr;
};
```

## 6. P2 待办

来自 spec Section 2.2,当前阶段未实现,留作后续迭代:

- **像素 diff 基线 / 视觉回归阈值** — manifest.json 已记 md5,可对基线仓库做 `md5 != baseline` 触发回归告警;进一步可接 ImageMagick `compare` 算 SSIM 阈值。
- **剪贴板自动识别 URL / 拖入种子 / 批量粘贴用例** — 需 ClipboardWatcher 抽象(见下)。
- **PluginManager / NetDiskManager / ClipboardWatcher 抽象** — 当前只抽象了 BrowserManager + Settings,后续把插件 / 网盘 / 剪贴板也抽接口 + Fake,才能在集成测试里覆盖更多业务路径。
- **CI 集成(GitHub Actions、上传 artifact)** — `ctest -L qml_ui` 跑通后把 `test_artifacts/qml_ui/` 打 zip 上传,PR review 可下载对比。
- **主题对比 HTML 报告** — 把 light/dark 同 case 的两张 PNG 并排嵌入 HTML,方便人工 review 主题一致性。

## 7. 已知限制

- **FramelessHelper offscreen ASSERT**:`tst_main_views` 中 `mainwindow_light` / `mainwindow_dark` 是空白 PNG(约 3674 bytes),因 FramelessHelper 在 offscreen 平台触发 ASSERT。`full_window_light` / `full_window_dark` 作为替代覆盖主窗口视觉。
- **Non-popup dialog chrome**:`GMessageBox` / `CloseConfirmDialog` / `DeleteConfirmDialog` 用 `contentItem` 重定向截图,缺标题栏装饰。Popup-based dialogs(`HelpDialog` / `UpdateDialog`)用 `captureWindow` 完整捕获。
- **Fake 独立 QObject**:`FakeBrowserManager` / `FakeSettingsManager` 不继承 `BrowserManagerImpl` / `SettingsImpl`(因 gdownload 是 .exe 不是可链接库,直接链会 LNK2019)。改为镜像接口 + `FAKE_SETTING` 宏。测试注册时直接 `qmlRegisterSingletonInstance<FakeXxx>`。
- **GTheme 测试用桩**:`tst_theme_toggle` 用 `TestGTheme` 桩验证 token 切换,因 gdownload.exe 的 GTheme 符号不可链接到独立测试。
- **2 个未注入 objectName**:`navHome`(开外链,无对应页面)、`navNetdisk`(无顶层导航入口);`tst_navigation` 仅验证 `navDownloading` 路由,其余导航项待对应页面落地后再补。
- **offscreen 字体 / 渲染差异**:offscreen 平台字体渲染与真实显示器可能有细微差异,像素 diff 阈值需留容差(建议 SSIM ≥ 0.95 而非字节级比对)。
- **Loader 异步加载**:`PageHarness` 用 `Loader` 装载页面,异步加载需 `wait(200)` 或 `tryCompare(harness, "isReady", true)` 等就绪后再截图,否则抓到空白。
