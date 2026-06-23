# Changelog

## v1.0.8

### 🚀 Highlights / 亮点
- Brand-new V5 interface: a redesigned home dashboard and a unified "Download Center" workbench built on the Element Plus design language.
- Introduced a complete GTheme design-token system (spacing, radius, typography, motion, layout and 4-level elevation shadows) for consistent light/dark theming.
- Reworked the download experience: compact task cards, a summary panel with quick-entry shortcuts, and top-bar filters replacing the legacy secondary sidebar.

### ✨ New & Improved / 新增与改进
- 全新首页仪表盘与下载中心工作台外壳，全面对齐 V5 设计系统。
- 新增下载中心摘要卡片与快捷入口卡片，下载列表改为紧凑任务卡片。
- 扩展通用组件库：卡片与 chip 按钮新增基础变体，新增 GElevation 阴影注入共享组件。
- 令牌化 TaskDialog 与 NetDisk 解析态/文件列表界面，统一视觉与交互。
- 主色、状态色与明暗中性色全面对齐 Element Plus 色板。

### 🐛 Bug Fixes / Bug 修复
- 修复下载中心卡片区域高度坍缩、空状态插图尺寸过大等布局问题。
- 修正下载导航语义、筛选按钮选中态与快捷入口卡片 hover 反馈。
- 加固核心配置、插件与异步生命周期处理，提升运行稳定性。

### 🧪 Testing / 测试
- 新增 QML UI 自动化测试框架：可视化截图测试（主视图 / 设置页 / 对话框）、ScreenshotHelper 与集成用例。
- 抽取 ISettings / IBrowserManager 接口，引入测试模式开关与 manager 工厂注入，提升可测试性。

### 🛠 Build & CI / 构建与 CI
- CI（cli-matrix）新增 concurrency 控制，按 ref 取消重复运行，避免半成品发布。

---

**Full Changelog**: https://github.com/cool2528/GDownload/compare/v1.0.7...v1.0.8<br>
**完整变更日志**: https://github.com/cool2528/GDownload/compare/v1.0.7...v1.0.8
