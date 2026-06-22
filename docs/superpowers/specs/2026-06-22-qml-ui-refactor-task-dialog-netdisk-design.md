# QML UI 重构 TaskDialog + NetDisk 批次设计

- 日期: 2026-06-22
- 上游 spec: `2026-06-19-qml-ui-refactor-foundation-design.md`、`2026-06-19-qml-ui-refactor-settings-pages-design.md`
- 范围: 添加任务对话框 TaskDialog 系列与其内嵌 Baidu NetDisk 页面令牌化
- 推进方式: 令牌化 + 轻量结构收敛,不改业务接口,不新增大型公共组件
- 设计语言: Element Plus 风格,沿用 `GTheme`、`GCard`、`GButton`、`Divider`、`AlertTip`、`GElevation`

## 0. 调查结论

本批次继续遵循上游 spec 的强制流程:先读相关代码,按真实用途确定范围,避免按文件名猜测。

已核查的核心文件:

- `src/App/ui/Resource/qml/CommonComponents/TaskDialogPage.qml`
- `src/App/ui/Resource/qml/CommonComponents/TaskDialogHeader.qml`
- `src/App/ui/Resource/qml/CommonComponents/NetDiskPageView.qml`
- `tests/qml_ui/integration/tst_create_task.cpp`
- `tests/qml_ui/visual/tst_main_views.qml`
- `tests/qml_ui/visual/tst_dialogs.qml`

现状结论:

1. `TaskDialogPage.qml` 是 `Popup` 型添加任务对话框,包含 URL、Torrent、Baidu 三个 tab,其中 Baidu tab 直接嵌入 `NetDiskPageView`。
2. `TaskDialogHeader.qml` 是 TaskDialog 专用头部,当前由调用方传入 `headerHeight`、`standardPadding`、`standardSpacing`,自身没有完整令牌化布局。
3. `NetDiskPageView.qml` 既可被 qml_ui 视觉测试单独加载,也被 TaskDialog 的 Baidu tab 嵌入;如果只重构 TaskDialog 外壳,Baidu tab 会保留明显旧视觉。
4. 现有 qml_ui 已覆盖 `NetDiskPageView` light/dark 截图,并通过 `tst_create_task` 覆盖 TaskDialog URL 创建任务路径。
5. 本批次不需要新增 NetWorkDiskManager Fake。NetDisk 深度业务集成测试仍属于 qml_ui P2 待办。

## 1. 范围

### 1.1 范围内

本批次只聚焦添加任务对话框路径:

1. `TaskDialogPage.qml`
   - 对话框外壳
   - URL / Torrent / Baidu 三个 tab 容器
   - footer 按钮区
   - 与 `BrowserManager.CreateTask` 相关的现有提交流程

2. `TaskDialogHeader.qml`
   - 标题区
   - 图标块
   - 关闭按钮

3. `NetDiskPageView.qml`
   - Baidu tab 内嵌网盘解析页
   - 初始解析态
   - 文件列表态
   - footer 返回/重新解析按钮

4. 复用已有组件和令牌
   - `GTheme`
   - `GCard`
   - `GButton`
   - `Divider`
   - `AlertTip`
   - `GElevation`

5. 验证
   - 保持现有 `NetDiskPageView` 截图用例可加载
   - 保持现有 `tst_create_task` 集成用例通过
   - 视实现情况补 TaskDialog popup light/dark 截图用例

### 1.2 范围外

本批次不做以下事项:

1. 不改 C++ manager 接口
   - 不抽象 `NetWorkDiskManager`
   - 不新增 `PluginManager`、`NetDiskManager`、`ClipboardWatcher` Fake
   - 不改变 `BrowserManager.CreateTask` 参数结构

2. 不重写网盘业务流程
   - `ParseShareUrl`
   - `ChangeDir`
   - `SelectAll`
   - `UnselectAll`
   - `ToggleSelection`
   - `GetDownloadInfo`

3. 不拆大型子组件
   - 不把 URL / Torrent / Baidu 三页拆成独立 QML 文件
   - 不新增 `GTaskDialog`、`GTabBar`、`GFileList` 等公共组件

4. 不做 qml_ui P2 增强
   - 不做视觉 diff 基线
   - 不做 CI artifact 上传
   - 不做主题对比 HTML 报告
   - 不做 NetDiskManager 深度 Fake 集成测试

## 2. 设计目标

1. 令牌化目标
   - 主要视觉值走 `GTheme`。
   - 消除目标文件中的旧式散落视觉魔法数字,例如裸字号、间距、圆角、阴影参数。
   - 保留必要页面级布局常量,但必须集中命名并用中文注释说明用途。

2. 视觉一致性目标
   - TaskDialog 外壳、头部、内容卡片、footer 与前序批次的对话框和设置页风格一致。
   - URL / Torrent / Baidu 三个 tab 在同一对话框内视觉统一。
   - `NetDiskPageView` 作为单独视觉测试页加载时也应符合令牌体系。

3. 行为保持目标
   - 不改变创建任务、剪贴板、拖种子、网盘解析、目录切换、选择文件等业务行为。
   - 保留现有测试依赖的 `objectName`。
   - 不改变用户可见文本含义;新增或调整 UI 文本必须为英文并用 `qsTr()` 包裹。

## 3. TaskDialogPage 设计

### 3.1 页面级布局常量

以下属于 TaskDialog 自身布局,允许作为集中命名常量保留:

- `dialogWidth: 720`
- `contentMinHeight: 460`
- URL / Torrent / Baidu 面板的期望高度
- 需要时的 `dialogMaxHeightMargin`

要求:

- 不散落裸数字。
- 常量集中在文件顶部。
- 中文注释说明它们是页面级布局常量,不是通用设计令牌。

### 3.2 视觉令牌

以下必须迁移到 `GTheme` 或现有组件:

- padding / spacing: `GTheme.space*`
- radius: `GTheme.radiusBase` 或 `GTheme.radiusLarge`
- 字号: `GTheme.fontCaption`、`fontBody`、`fontSubtitle`、`fontTitle`
- 按钮高度: `GTheme.sizeDefault`
- 阴影: 优先复用 `GElevation` 或 `GTheme.elevation*`
- 动效: `GTheme.durationBase` + `GTheme.easingStandard`

旧式 `DropShadow` + `Qt.rgba(0, 0, 0, 0.1)` 不再保留。

### 3.3 Tab 区域

当前 URL / Torrent / Baidu 三个 tab 已使用 `GButton { variant: "nav" }`,方向正确。本批次只收敛布局:

- tab 容器高度改为命名常量或由 `GTheme.navItemHeight + GTheme.spaceSM` 表达。
- tab 间距使用 `GTheme.spaceXS` 或 `GTheme.spaceSM`。
- 不新增全局 `GTabBar`。

### 3.4 内容卡片

三个 tab 内容都应使用同一套容器规则:

- 外层 `GCard { outlined: true }`
- padding 使用 `GTheme.spaceSM` 或 `GTheme.spaceMD`
- URL 输入框背景、边框、圆角、focus 行为令牌化
- Torrent 区域不改 `GDropArea` / `FilePreviewList` 业务,只统一容器尺寸和 padding
- Baidu 区域继续嵌入 `NetDiskPageView`

## 4. TaskDialogHeader 设计

`TaskDialogHeader.qml` 从“由调用方传视觉参数”改为“自包含令牌化头部”。

保留的外部 API:

- `title`
- `subtitle`
- `closeRequested()`

移除调用方视觉传参:

- `headerHeight`
- `standardPadding`
- `standardSpacing`

头部内部规则:

- 根高度使用页面级命名常量或 `GTheme.titleBarHeight` 附近的明确表达,由实现阶段按实际视觉选择。
- 左右边距使用 `GTheme.space2XL` 或 `GTheme.spaceLG`。
- 图标容器尺寸使用 `GTheme.sizeLarge` 或局部 `iconBoxSize`。
- 图标字号使用 `GTheme.fontTitle`。
- 标题使用 `GTheme.fontTitle` + `GTheme.weightDemiBold`。
- 副标题使用 `GTheme.fontBody`。
- 关闭按钮继续使用 `GButton` icon-only 模式。

调用方目标形态:

```qml
TaskDialogHeader {
    onCloseRequested: taskPage.close()
}
```

## 5. NetDiskPageView 设计

### 5.1 状态模型

保持现有状态源:

- `topBar.visible === true`:初始解析态
- `topBar.visible === false`:文件列表态

本批次不引入第二套状态字段,避免状态不同步。

### 5.2 初始解析态

顶部输入行:

- 使用 `RowLayout`。
- 输入框高度使用 `GTheme.sizeDefault` 或 `GTheme.sizeLarge`。
- 输入框文字使用 `GTheme.fontCaption` 或 `GTheme.fontBody`,按可读性选择。
- 输入框背景、边框、圆角走 `GTheme`。
- Parse 按钮继续使用 `GButton type: 1`。

注意事项区域:

- 优先复用 `AlertTip` 表达 warning/danger 提示。
- 如果 `AlertTip` 单段文本不适合多行列表,则在 `NetDiskPageView.qml` 内使用 `GCard + ColumnLayout` 写局部提示卡片。
- 提示卡片颜色取 `GTheme.bgDanger`、`borderDanger`、`textDanger` 或更合适的 warning 语义令牌。
- 不新增全局提示组件。

设置跳转按钮:

- 保持现有行为:设置 `brower_view.index = 1` 并调用 `brower_view.switchSettingPage(1)`。
- 视觉尺寸走 `GTheme.sizeDefault`。

### 5.3 文件列表态

列表 header、delegate、footer 统一命名和令牌化:

- `rowHeight: GTheme.sizeDefault`
- `fileIconSize`:页面级布局常量或 `GTheme.fontTitle`
- `fileNameMinWidth`
- `fileSizeMinWidth`
- `fileDateMinWidth`

说明:

- 列宽属于表格布局约束,允许作为页面级布局常量保留。
- 行文字使用 `GTheme.fontBody`。
- 行间距使用 `GTheme.spaceXS` 或 `GTheme.spaceSM`。
- 不强行新增 hover、选中背景或复杂交互,避免改变行为。

## 6. 行为保持

### 6.1 TaskDialogPage

以下行为必须保持:

- 打开对话框时 URL 输入框读取 `ClipboardWatcher.GetClipboardText()`。
- `ClipboardWatcher.onClipboardChanged` 仍只在 `taskPage.visible && text.length > 3` 时更新输入框。
- URL tab 提交时继续使用 `Utils.splitPath(input.text)` 并设置 `urlType = 0`。
- Torrent tab 提交时继续使用 `dropTorrent.path`,并按扩展名区分 metalink/meta4 与 torrent。
- Create 按钮继续调用现有创建任务路径。
- 创建成功后继续切换到下载页。
- Cancel 按钮只关闭 popup。
- 保留 `objectName: "inputUrl"` 和现有测试依赖的按钮 objectName。

### 6.2 NetDiskPageView

以下行为必须保持:

- URL 校验函数 `checkShareUrl(url)` 的语义不变。
- cookie 为空仍通过 `ToastManager.ShowError(qsTr("Please set Baidu Netdisk cookies first."))` 提示。
- 解析调用仍为 `NetWorkDiskManager.ParseShareUrl(shareUrl)`。
- `onTaskFinished` 的 taskType 分支语义不变。
- 文件夹点击仍调用 `NetWorkDiskManager.ChangeDir(model.filePath, model.fileId)`。
- 全选/取消全选仍调用 `SelectAll()` / `UnselectAll()`。
- 单行选择仍调用 `ToggleSelection(index, !model.isSelected)`,并保留当前 binding 恢复逻辑。
- Back / Return parsing 按钮行为不变。

## 7. 错误处理

本批次不新增 C++ 层错误处理,只保持并整理 QML 层现有提示:

- 继续使用 `ToastManager.ShowError` / `ToastManager.ShowSuccess`。
- 不把网盘错误改成 inline error,避免改变用户流程。
- URL 为空或格式不对继续走 invalid URL toast。
- cookie 未设置继续走 cookie toast。
- `NetWorkDiskManager` 返回失败继续走 `ToastManager.ShowError(msg)`。

允许的小修正:

- 删除确认无用的调试残留 `console.log("busyIndicator", ...)`。
- 保持 `checkShareUrl` 正则匹配范围,不扩大业务接受范围。

## 8. 验证策略

### 8.1 静态检查

针对目标文件检查:

- 无 `ElementPlusColors.` 直接调用。
- 无旧式 `Qt.rgba(0, 0, 0, 0.1)` 阴影。
- 无散落旧视觉魔法数字,如 `font.pixelSize: 13/15/18`、`spacing: 10/5`、`radius: 6/8`。
- 页面级布局常量集中命名,不散落。

### 8.2 QML lint

至少覆盖:

- `TaskDialogPage.qml`
- `TaskDialogHeader.qml`
- `NetDiskPageView.qml`

如项目现有验证方式使用 qml_ui 测试命令封装,按既有方式执行。

### 8.3 qml_ui 测试

必须保持通过:

- `tst_create_task`:验证 TaskDialog URL 创建路径未破坏。
- `tst_main_views`:验证 `NetDiskPageView` light/dark 截图仍可生成。

推荐补充:

- TaskDialog popup light/dark 截图用例,可加入 `tst_dialogs.qml` 或新增视觉用例。

不在本批次新增:

- NetWorkDiskManager Fake。
- 网盘列表业务集成测试。

### 8.4 编译

代码实现完成后按项目规范主动编译验证。若 CMake 配置未变化,优先复用现有 build 目录直接构建;如构建系统提示需重新配置,再执行 configure。

## 9. 回归风险与控制

| 风险 | 控制方式 |
|---|---|
| Popup 高度计算变化导致内容裁切 | 保留 `contentMinHeight`,截图验证 URL / Torrent / Baidu 初始状态 |
| NetDisk 初始态高度不足 | 提示区使用布局隐式高度或清晰的页面级高度常量,避免硬凑散落数值 |
| 测试依赖 objectName 丢失 | 明确保留 `inputUrl`、创建按钮等 objectName |
| 业务状态切换破坏 | 不改变 `topBar.visible` 作为解析态/列表态状态源 |
| 视觉过度重构 | 不拆 URL/Torrent/Baidu 子文件,不改 C++ manager,不新增大型公共组件 |

## 10. 实施顺序建议

1. 重构 `TaskDialogHeader.qml`,使其自包含并令牌化。
2. 重构 `TaskDialogPage.qml` 外壳、tab、URL/Torrent/Baidu 卡片、footer。
3. 重构 `NetDiskPageView.qml` 初始解析态。
4. 重构 `NetDiskPageView.qml` 文件列表态。
5. 运行静态检查、qmllint、相关 qml_ui 测试和编译。
6. 根据截图和测试结果做小修正。

## 11. 文档与翻译说明

- 不修改 `.ts` 翻译文件。
- 新增或调整 QML 用户可见文本必须使用英文并包裹 `qsTr()`。
- 完成实现后提醒用户运行翻译更新流程。

## 12. 不变量

实现完成后,以下不变量必须成立:

1. 用户仍可从 TaskDialog URL tab 创建普通下载任务。
2. 用户仍可拖入 torrent/metalink 并走现有预览/创建流程。
3. 用户仍可在 Baidu tab 输入分享链接并触发现有解析逻辑。
4. qml_ui 中 `NetDiskPageView` 单页截图仍可加载。
5. `tst_create_task` 仍可定位 URL 输入框并断言创建任务调用。
