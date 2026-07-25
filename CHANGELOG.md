# Changelog

## v2.1.0

### ✨ 更新体验 / Update Experience

- Check for updates silently on every startup (dialog appears only when a new version exists) and add a manual "Check for Updates" button in Settings with up-to-date / failure feedback. / 每次启动后静默检查更新（发现新版才弹窗），设置页新增手动检查按钮，提示"已是最新版本"或失败原因。
- Fix concurrent update checks racing each other: the manual check button could stay stuck on "Checking..." forever or pop up a toast it never asked for. / 修复启动自动检查与手动检查并发时按钮永久卡在 "Checking..." 或误弹提示的竞态。
- Fix false update-check results: a transient primary-server failure with a successful fallback no longer reports "check failed" on Windows, and failed checks on macOS are no longer reported as "up to date". / 修复误报：Windows 主源瞬时失败但备用源成功时不再误报"检查失败"；macOS 检查失败不再被误报为"已是最新版本"。

### 🛠 删除可靠性 / Deletion Reliability

- Make history deletion resilient after the aria2 engine restarts — removed tasks no longer resurrect from late notifications or recycled GIDs. / aria2 引擎重启后删除历史依然可靠，已删任务不再因迟到通知或 GID 复用而"复活"。
- Fix a counter wraparound that reported a successfully retried deletion as "not found". / 修复删除重试成功却被报告为"未找到"的计数回绕。
- Bulk deletion no longer retries locked files one by one, avoiding UI freezes when an antivirus briefly holds file handles. / 批量删除不再对被占用文件逐个重试退避，避免杀毒软件短暂占用句柄时界面冻结。

### ✨ eD2k

- Reliable server.met updates and Kad enabled by default. / server.met 更新更可靠，Kad 网络默认开启。

### 📦 安装与卸载 / Installer

- Uninstall can now optionally remove user data (settings, download history and plugin data — downloaded files are never touched), localized in all five installer languages; stale autostart entries are cleaned up; upgrades keep your settings. / 卸载时可选删除用户数据（设置、下载记录、插件数据，不影响已下载文件），提供 5 种语言文案；清理开机自启残留；升级安装不丢配置。

### 🔧 其他 / Misc

- Release notes now contain only the current version's changelog section. / Release Notes 只包含当前版本的变更章节。
- Faster release builds: vcpkg and sccache caches are kept warm between releases. / 发版构建提速：vcpkg 与 sccache 缓存跨发版保温。

## v2.0.0

### 🚀 Highlights / 亮点

- **Full eD2k / eMule client / 完整 eD2k 客户端** — download, search, server management, file sharing, and Kad DHT, powered by a self-developed in-process engine ([ed2k-engine-cpp](https://github.com/cool2528/ed2k-engine-cpp)). / 自研进程内引擎驱动的下载、搜索、服务器管理、文件分享与 Kad DHT。
- **Browser extension / 浏览器扩展** — native-messaging host with self-registration (Windows/macOS/Linux) and one-click link capture. / Native Messaging 自注册 + 一键捕获下载链接。
- **More cloud drives / 更多网盘** — Baidu, Aliyun Drive, Quark, 123 Pan, and Lanzou, plus an online plugin marketplace and development SDK. / 百度、阿里云盘、夸克、123、蓝奏,在线插件市场 + 开发 SDK。
- **Aurora UI** — a new design system and brand identity. / 全新 Aurora UI 设计系统与品牌标识。

### ✨ eD2k / eMule

- ed2k 链接下载:多源流水线、上传队列排队等待、断点续传、Kad 源发现。
- 搜索(服务器 + Kad 网络)、服务器管理(连接/增删/更新 server.met/Kad 状态)、文件分享(上传统计 + 队列)。
- 数据完整性与安全:损坏 part 重下、真 MD4 完成判定、hashset 防伪造、大文件 64 位、单包 DoS 防护。

### ✨ 网盘 / Cloud Drives

- 百度网盘、阿里云盘、夸克、123 网盘、蓝奏云解析;在线插件市场(Git 仓库即注册表 + Ed25519 签名)+ JS 插件开发 SDK。

### ✨ 浏览器扩展 / Browser Extension

- Native Messaging host + 自注册、扩展配对、单实例守卫、外部唤起接管解析。

### ✨ 平台与打包 / Platform & Packaging

- Windows 安装包多语言 + 自动检测系统语言(English / 简体中文 / 繁體中文 / 日本語 / 한국어)。
- 免费签名方案:ed25519 更新清单签名 + AppImage GPG(不使用付费代码签名)。
- 跨平台构建修复(Linux `<algorithm>` 缺失、`*.sh` 强制 LF 行尾)。

### 🎨 界面 / UI

- Aurora UI 设计系统 + 品牌 logo;首页"快速开始"新增 eD2k 入口;修正对话框种子 tab 等翻译;侧边栏图标放大。

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
