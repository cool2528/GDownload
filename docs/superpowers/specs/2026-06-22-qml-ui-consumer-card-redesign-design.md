# QML UI 整体重设计 — 跨平台高级消费级工作台

- 日期: 2026-06-22
- 范围: 基于现有 QML/GTheme/CommonComponents,将 GDownload 客户端整体视觉升级为跨平台统一品牌风的高级消费级工作台
- 视觉基准: Visual Companion 概念稿 v5 主窗口双主题 + NetDisk/Settings/Dialogs 双主题细化稿
- 设计方向: 消费级卡片型,但保留下载工具效率;Light/Dark 双主题同等设计;跨平台统一品牌风
- 推进方式: 分批 spec/plan/实现;每批以 qml_ui 自动化截图和集成测试验收

## 0. 背景与当前自动化基线

当前项目已具备可支撑系统性 UI redesign 的自动化设施:

- `qml_ui_visual`:Qt Quick Test 视觉截图,产物写入 `build/test_artifacts/qml_ui/`
- `qml_ui` 集成测试:QtTest C++ 加载真实 QML + Fake 后端,验证关键用户路径
- 当前 `ctest --test-dir build -C Debug -L qml_ui --output-on-failure` 为 9/9 通过
- 当前 Debug 全量构建通过
- `manifest.json` 已记录 light/dark 截图、md5、文件路径

已覆盖截图:

- 主视图:DownloadPageView / NetDiskPageView / NavigatorView / full_window / mainWindow 空白限制截图
- 设置页:14 个设置相关场景 × light/dark
- Lab 子卡片:4 个子卡片 × light/dark
- 对话框:CloseConfirm / Help / Update / DeleteConfirm / GMessageBox × light/dark

已知自动化边界:

1. `mainWindow_light/dark` 因 FramelessHelper offscreen 限制为空白;主布局用 `full_window_light/dark` 和页面级截图替代。
2. NetDisk 文件列表态尚无真实数据模型截图。
3. TaskDialog popup 视觉截图尚未单独作为 baseline。
4. 尚未实现 HTML 对比报告、baseline diff、SSIM 阈值。

这些限制不阻塞 redesign,但需要在对应批次补齐。

## 1. 设计定位

正式 UI redesign 采用:

> 跨平台统一品牌风 + 高级消费级下载工作台

这不是纯后台控制台,也不是只追求好看的大卡片概念页。它的产品目标是:

- 普通用户觉得友好、清晰、现代。
- 重度下载用户仍然能高效管理队列。
- Windows/macOS/Linux 视觉一致,形成 GDownload 自有品牌风。
- Light/Dark 双主题都能长期使用。
- 通过自动化截图持续验证视觉回归。

## 2. 核心产品原则

### 2.1 任务优先

GDownload 首页的主角仍是下载队列。状态总览、Quick Actions、欢迎文案都不能压过任务列表。

设计要求:

- 首页采用 v5 的横向 summary strip,不采用大面积 dashboard 卡片。
- 下载任务采用紧凑卡片,默认一屏至少能看到 3 条任务。
- 任务卡片必须直接展示速度、进度、状态和常用操作。

### 2.2 消费级但不低效

消费级视觉用于降低工具软件冷硬感,不是降低信息密度。

设计要求:

- 用圆角、柔和边框、低饱和状态背景、卡片分组提升亲和力。
- 避免过多大渐变、过重阴影、过大卡片。
- 支持未来 Comfort / Compact 两种密度策略。第一批可先实现 Comfort,但布局不能阻碍 Compact。

### 2.3 Light/Dark 同步设计

所有 UI 方案和实现验收必须 light/dark 同时考虑。

设计要求:

- 后续所有视觉稿、spec 验收、截图检查都必须同时列 light/dark。
- Dark 不是 Light 的简单反色,需要独立定义 background/surface/border/text/status 层级。
- 状态色在 dark 下不得刺眼。

### 2.4 跨平台统一品牌风

GDownload 在 Windows/macOS/Linux 上应保持一致品牌视觉。

设计要求:

- 不依赖平台原生 Controls 外观。
- 不按平台分叉视觉风格。
- 不使用 emoji 作为正式图标。
- 正式图标使用 `FontIcon` / `SegoeFluentIcons` / 项目统一 SVG/PNG 资源。
- 字体层级走 `GTheme.font*` 和 `GTheme.weight*`,不绑定平台字体特性。

### 2.5 基于现有真实组件演进

本 redesign 必须基于当前代码中的真实 QML 组件,不另起一套 UI 框架。

设计要求:

- 先扩展 `GTheme` 与 `CommonComponents`。
- 页面实现必须基于真实 QML 文件和现有数据流。
- 新增组件必须基于 `GCard` / `GButton` / `GTheme`,且有明确复用场景。

## 3. 现有组件基线

正式实现优先复用和扩展以下组件。

### 3.1 容器/卡片

- `GCard.qml`
- `SettingCard.qml`
- `GElevation.qml`

策略:

- 卡片型 UI 优先增强 `GCard`。
- `SettingCard` 继续服务设置页表单/设置卡片。
- 阴影统一使用 `GElevation` / `GTheme.elevation*`,不手写 `DropShadow + Qt.rgba(...)`。

### 3.2 按钮/入口

- `GButton.qml`
- `GImageButton.qml`
- `GButtonSwitch.qml`

策略:

- 主按钮、chip、Quick Actions、导航入口尽量基于 `GButton`。
- 如果需要 chip 风格,优先扩展 `GButton.variant: "chip"`。
- Quick Action 可基于 `GCard` + `MouseArea` 或扩展后的 `GButton`,实现前按真实调用点决定。

### 3.3 表单/设置

- `SettingCard.qml`
- `SettingRow.qml`
- `SettingFormActions.qml`
- `GTextField.qml`
- `GSpinBox.qml`
- `GComBoBox.qml`
- `GCheckBox.qml`

策略:

- 设置首页产品化为设置中心。
- 设置子页继续保持高效表单,不强行大卡片化。
- 不改 `SettingsManager` 接口。

### 3.4 下载/任务

- `GDownloadViewPage.qml`
- `GProgressBar.qml`
- `TaskDialogPage.qml`
- `TaskDialogHeader.qml`
- `TaskGeneralOptionsCard.qml`
- `TaskAdvancedOptionsCard.qml`
- `FilePreviewList.qml`
- `GDropArea.qml`

策略:

- 下载任务卡片必须围绕 `GDownloadViewPage.qml` 的真实 delegate 和模型字段设计。
- 第一批优先在现有 delegate 内卡片化,只有确认复用和边界后再抽 `DownloadTaskCard.qml`。

### 3.5 NetDisk

- `NetDiskPageView.qml`

策略:

- 云盘页演进为三步流程:Paste link / Preview files / Add queue。
- 保持现有状态源:`topBar.visible === true` 为解析态,`topBar.visible === false` 为文件列表态,除非另立状态机 spec。
- 不抽象 `NetWorkDiskManager` 作为 UI redesign 的前置。

### 3.6 对话框/反馈

- `GDialogShell.qml`
- `GMessageBox.qml`
- `GMessage.qml`
- `ToastContainer.qml`
- `AlertTip.qml`
- `CloseConfirmDialog.qml`
- `DeleteConfirmDialog.qml`
- `HelpDialog.qml`
- `UpdateDialog.qml`

策略:

- 不新增第二套 Message 系统。
- 不恢复已删除的 MessageManager。
- 只 polish 现有反馈体系。

## 4. 视觉 Token 策略

### 4.1 不重建颜色系统

继续沿用当前 `GTheme` / `elementPlusColors.h` / Element Plus 色阶 / 语义告警 token。

禁止:

- 新建完整品牌色阶。
- 页面私有大段 `#xxxxxx`。
- `Qt.rgba(...)` 透明色硬编码。
- 平台条件色值。
- emoji 图标。

### 4.2 现有 token 使用规范

| 语义 | Token | 用法 |
|---|---|---|
| Primary | `primaryColor`, `primaryLight(7/8/9)` | 主按钮、选中态、URL 入口、导航 active、进度条主色 |
| Success | `successColor`, `bgSuccess`, `textSuccess` | 下载速度、完成状态、成功 toast |
| Warning | `warningColor`, `bgWarning`, `textWarning` | 等待队列、提示、queued |
| Info | `infoColor`, `bgInfo`, `textInfo` | 普通辅助信息、neutral status |
| App background | `bgPage` | 主窗口内容背景 |
| Surface/Card | `bgWhite` / `bgBase` / `bgElevated` | 卡片、面板 |
| Subtle surface | `fillLighter` / `fillLight` | 输入框、内嵌区、摘要行 |
| Border | `borderLight` / `borderBase` | 卡片边框、focus、selected |

### 4.3 可选新增语义 token

只有现有 token 无法表达 v5 的 light/dark 层级时,才新增少量 token。

候选:

- `surfaceMuted`
- `surfaceAccentPrimary`
- `surfaceAccentSuccess`
- `surfaceAccentWarning`
- `surfaceAccentInfo`

用途:

- summary strip 背景
- 设置摘要行
- task card 内部浅色块
- Quick Action 低饱和背景
- dark 下低饱和状态背景

实现前必须先核实现有 `bgSuccess/bgWarning/bgInfo/primaryLight` 是否足够,能复用则不新增。

## 5. 组件 API 策略

### 5.1 GCard 增强

建议为 `GCard` 增加:

```qml
property string variant: "default"
// default | muted | elevated | accentPrimary | accentSuccess | accentWarning | accentInfo

property bool interactive: false
property bool compact: false
```

用途:

- `default`:普通卡片
- `muted`:浅色内嵌块
- `elevated`:重要卡片
- `accentPrimary`:主色入口/选中
- `accentSuccess`:速度/完成
- `accentWarning`:等待/提示
- `accentInfo`:中性信息

约束:

- 颜色全部映射到 `GTheme`。
- `interactive` 控制 hover/cursor/press feedback。
- `compact` 使用更小 padding/radius。

### 5.2 GButton 增强

建议扩展:

```qml
variant: "chip"
```

用途:

- Pause
- Open
- Details
- 小型操作按钮
- 下载任务卡片右侧操作

约束:

- 不新增 `GChipButton.qml`,除非调用点很多且行为复杂。
- chip 高度和 padding 走 `GTheme`。

### 5.3 SummaryMetricCard

建议新增:

`src/App/ui/Resource/qml/CommonComponents/SummaryMetricCard.qml`

API 草案:

```qml
SummaryMetricCard {
    title: qsTr("Current speed")
    value: "24.8"
    unit: "MB/s"
    iconSource: SegoeFluentIcons.Download
    accent: "primary" // primary | success | warning | info
}
```

约束:

- 基于 `GCard`。
- 不直接写颜色。
- 支持 light/dark。
- 可用于首页 summary strip,未来可用于 NetDisk/Settings 摘要。

### 5.4 QuickActionCard

建议新增:

`src/App/ui/Resource/qml/CommonComponents/QuickActionCard.qml`

API 草案:

```qml
QuickActionCard {
    title: qsTr("URL")
    description: qsTr("Paste download links")
    iconSource: SegoeFluentIcons.Link
    accent: "primary"
    onClicked: ...
}
```

约束:

- 基于 `GCard`。
- `interactive: true`。
- 不改 TaskDialog 创建逻辑。
- 点击后调用现有 TaskDialog/tab 逻辑。

### 5.5 DownloadTaskCard 谨慎新增

推荐第一批先不新增,优先在 `GDownloadViewPage.qml` 现有 delegate 内卡片化。

只有满足以下条件再抽组件:

- 已明确真实模型字段。
- 已明确 pause/open/details 等 action 信号。
- 抽出后不会破坏 delegate 上下文绑定。

### 5.6 SettingsCategoryCard

建议新增:

`SettingsCategoryCard.qml`

用途:

- 设置中心首页四大类入口。

API 草案:

```qml
SettingsCategoryCard {
    title: qsTr("Speed & Queue")
    description: qsTr("Concurrent downloads, speed limit, auto start")
    summary: qsTr("Concurrent: 5")
    iconSource: SegoeFluentIcons.SpeedHigh
    accent: "primary"
    onClicked: ...
}
```

约束:

- 基于 `SettingCard` 或 `GCard`。
- 不替换设置子页表单。

## 6. 分批实施范围

### 6.1 批次 1:主窗口与下载中心

目标:

- 实现 v5 的高级下载工作台。
- Summary strip + 紧凑任务卡片 + Quick Actions。

真实文件:

- `Browser/DownloadPageView.qml`
- `Browser/DownloadPageTitle.qml`
- `CommonComponents/GDownloadViewPage.qml`
- `Navigator/NavigatorView.qml`
- `mainWindow.qml`
- `CommonComponents/GProgressBar.qml`
- `CommonComponents/GCard.qml`
- `CommonComponents/GButton.qml`

可能新增:

- `SummaryMetricCard.qml`
- `QuickActionCard.qml`

暂不新增:

- `DownloadTaskCard.qml`(先在现有 delegate 内卡片化)

不做:

- 不改下载业务模型。
- 不改 aria2 manager。
- 不改 Frameless 主窗口逻辑。

### 6.2 批次 2:TaskDialog 与 NetDisk 工作流

目标:

- TaskDialog 三入口产品化。
- NetDisk 三步流程产品化。

真实文件:

- `CommonComponents/TaskDialogPage.qml`
- `CommonComponents/TaskDialogHeader.qml`
- `CommonComponents/TaskGeneralOptionsCard.qml`
- `CommonComponents/TaskAdvancedOptionsCard.qml`
- `CommonComponents/NetDiskPageView.qml`
- `CommonComponents/FilePreviewList.qml`
- `CommonComponents/GDropArea.qml`

可能新增:

- `WorkflowStepCard.qml`
- `TaskSourceCard.qml`

前提:

- 至少两个场景复用才新增。

不做:

- 不抽象 `NetWorkDiskManager`。
- 不改变 `topBar.visible` 状态源。
- 不改 `checkShareUrl` 正则。
- 不改 `ClipboardWatcher` 行为。
- 不改 `BrowserManager.Add*Task` 调用。

### 6.3 批次 3:Settings 设置中心

目标:

- 设置首页产品化为设置中心。
- 设置子页保持高效表单。

真实文件:

- `Browser/SettingsPageView.qml`
- `Browser/SettingPageTitle.qml`
- `BasicSettingPage.qml`
- `AdvancedSettingPage.qml`
- `LabSettingPage.qml`
- `SpeedControlSettingPage.qml`
- `ConnectionPerformanceSettingPage.qml`
- `TimeoutRetrySettingPage.qml`
- `BitTorrentAdvancedSettingPage.qml`
- `UserAgentSettingPage.qml`
- `PostDownloadActionsSettingPage.qml`
- `BaiduCookieSettingPage.qml`
- `TrackerServerSettingPage.qml`
- `Aria2RpcSettingPage.qml`
- `SettingCard.qml`
- `SettingRow.qml`
- `SettingFormActions.qml`
- `AlertTip.qml`

可能新增:

- `SettingsCategoryCard.qml`
- `SettingsSummaryRow.qml`(仅复用明确时)

不做:

- 不改 `SettingsManager` 接口。
- 不重命名设置项。
- 不改 `.ts`。

### 6.4 批次 4:Dialogs / Feedback

目标:

- 产品化对话框和反馈系统。
- Toast / MessageBox / AlertTip 状态色一致。

真实文件:

- `GMessage.qml`
- `ToastContainer.qml`
- `GMessageBox.qml`
- `GDialogShell.qml`
- `CloseConfirmDialog.qml`
- `DeleteConfirmDialog.qml`
- `HelpDialog.qml`
- `UpdateDialog.qml`
- `AlertTip.qml`

不做:

- 不新增第二套 Message 系统。
- 不恢复 MessageManager。
- 不改 ToastManager C++ 接口。
- 不改 CloseConfirm 业务行为。

### 6.5 批次 5:视觉回归工具增强

目标:

- 把自动截图升级为设计 review 工具。

范围:

- HTML 对比报告
- light/dark 并排
- manifest diff
- baseline md5 / SSIM
- PR artifact 上传

可能新增:

- `tests/qml_ui/tools/generate_visual_report.py`
- `tests/qml_ui/tools/compare_baseline.py`

不做:

- 不阻塞批次 1。
- 不一开始强制像素级 diff。

## 7. 自动化截图验收标准

### 7.1 全局验收

每个 UI 批次完成后必须运行:

```bash
ctest --test-dir build -C Debug -L qml_ui --output-on-failure
```

并确认:

- 测试通过。
- light/dark 截图均存在。
- PNG 非空白。
- manifest.json 生成。

### 7.2 批次 1 验收

必须检查:

- `download_light`
- `download_dark`
- `full_window_light`
- `full_window_dark`
- `navigator_light`
- `navigator_dark`

人工验收点:

- summary strip 不占过多高度。
- 任务卡片一屏至少显示 3 条。
- light/dark 卡片层级清楚。
- 进度条颜色不刺眼。
- 导航选中态明确。
- Quick Actions 不抢主列表焦点。

### 7.3 批次 2 验收

必须检查:

- `netdisk_light`
- `netdisk_dark`
- `tst_create_task`

建议补:

- `taskdialog_light`
- `taskdialog_dark`
- `netdisk_list_light`
- `netdisk_list_dark`

人工验收点:

- URL/Torrent/Baidu 入口清楚。
- NetDisk 三步流程不占太多空间。
- 文件列表仍紧凑。
- dark 下输入框和列表边界清楚。

### 7.4 批次 3 验收

必须检查:

- `tst_settings_pages` 28 张截图
- `tst_save_settings`

人工验收点:

- 设置首页像设置中心。
- 设置子页仍高效。
- 卡片分组明确。
- 页面不过长。
- 表单控件高度统一。

### 7.5 批次 4 验收

必须检查:

- `tst_dialogs`
- `tst_close_confirm`

人工验收点:

- 对话框不像系统弹窗。
- Toast 与 MessageBox 状态色一致。
- Help / Update 内容可读。
- dark 下阴影和边框不过重。

## 8. 成功标准

整体 redesign 成功应满足:

1. 视觉统一:主页面、设置、对话框像同一个产品。
2. 效率不下降:下载列表、设置、任务创建仍高效。
3. 跨平台一致:不依赖原生控件、emoji、平台字体特性。
4. Light/Dark 同等质量:暗色不是简单反色。
5. 自动化可验证:qml_ui 通过,截图覆盖主要页面,关键行为测试不破。
6. 基于真实组件演进:没有概念稿式孤立实现。

## 9. 近期推荐执行

下一步建议单独为 **批次 1:主窗口与下载中心** 写 implementation plan。

批次 1 的设计重点:

- 先读 `GDownloadViewPage.qml` delegate 和下载模型字段。
- 判断是否在现有 delegate 内卡片化,还是抽 `DownloadTaskCard`。
- 增强 `GCard` / `GButton`。
- 新增 `SummaryMetricCard` / `QuickActionCard`。
- 更新 `DownloadPageView` / `DownloadPageTitle` / `NavigatorView`。
- 更新或补充 qml_ui 截图用例。

## 10. 翻译与文档规则

- 不修改 `.ts` 翻译文件。
- 新增 QML 用户可见文本必须使用英文并包裹 `qsTr()`。
- 完成实现后提醒用户运行翻译更新流程。
- 若新增核心组件或页面结构变更,按项目规范更新 `docs/CODEBASE_EXPLORATION_REPORT.md` 的相关组件清单。
