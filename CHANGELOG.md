# Changelog

## v2.2.0

### ⚡ 界面卡顿 / UI Responsiveness

- **Fix multi-second UI freezes (occasionally "Not Responding") while dragging the window, especially with downloads running.** Every settings write went straight to disk on the UI thread — rewriting the whole config file and waiting for a physical disk flush — and dragging the window persists its position twice per pause. With a download saturating the disk, that flush queued behind it. Settings are now written by a background thread that coalesces bursts. Measured on the same machine with downloads active (900 simulated drags): worst-case stall went from **17.8 seconds to 7.7 milliseconds**, with zero dropped frames. / **修复拖动窗口时数秒级的界面冻结(偶尔直接显示"未响应"),下载进行时尤其明显。** 每次设置写入都在 UI 线程上直接落盘 —— 重写整份配置文件并等待物理磁盘刷新 —— 而拖动窗口每次停顿都会保存两次窗口位置;下载把磁盘占满时,那次刷新就得排队等。现在配置由后台线程合并后写入。同机同负载实测(900 次模拟拖动):最大卡顿从 **17.8 秒降到 7.7 毫秒**,零掉帧。

- **A newly added download now appears in the list immediately instead of after 5–10 seconds.** The gid returned by aria2 when the task was created was being discarded, so the new row had to wait for the next polling cycle. / **新添加的下载任务立即出现在列表中,不再等 5~10 秒。** aria2 创建任务时返回的 gid 被丢弃了,新任务只能等下一轮轮询才被发现。
- **Fix the window opening in the top-left corner on first launch — it is now centered.** A never-saved window position was mistaken for a saved `(0,0)`. A position saved on a monitor that is no longer connected is also ignored now, instead of putting the window off-screen. / **修复首次启动时窗口贴在桌面左上角,现在正常居中。** 从未保存过的窗口位置被误当成保存过的 `(0,0)`。此外,保存于已拔掉的显示器上的位置现在也会被忽略,不再把窗口放到屏幕之外。

### 🔍 eD2k 搜索 / eD2k Search

- **Search now asks every server in your list instead of just the one you are connected to.** Each eD2k server keeps its own independent filename index, so searching a single server returned only a small slice of what is out there. Measured on a real 8-server list with the keyword `ubuntu`: the connected server returned 52 results, the union across all eight was **724**. Results from the first server appear as fast as before; the rest are fetched in the background and appended as each server answers. / **搜索改为向服务器列表里的每一台发起,而不只是当前连接的那一台。** eD2k 每台服务器的文件名索引互相独立,只搜一台会系统性漏掉绝大部分结果。真实 8 台服务器实测(关键词 `ubuntu`):已连接那台 52 条,八台并集 **724 条**。首屏结果出现的速度不变,其余服务器的结果在后台陆续追加。
- The result list keeps showing "searching" until every server has answered, so a slow server can no longer make an in-progress search look like it found nothing. / 搜索期间界面持续显示"搜索中"直到全部服务器答完,不会再出现"先说没有结果、十几秒后结果又冒出来"。

### ⬇️ eD2k 下载 / eD2k Downloads

引擎从 2.7.4 升级到 2.12.1,期间完成了一轮针对 eMule/aMule 线协议的全面对齐(122 项审计结论)。用户可感知的部分: / The engine moved from 2.7.4 to 2.12.1, which included a full wire-protocol alignment pass against eMule/aMule (122 audited findings). What you can notice:

- **Files whose size is an exact multiple of 9,728,000 bytes can now complete.** They previously re-downloaded the last chunk forever. / **大小恰好是 9,728,000 字节整数倍的文件现在能下完了** —— 此前最后一块会无限重下。
- **Small files (≤180 KB) with an AICH hash in the link no longer always fail.** / **链接里带 AICH 校验的小文件(≤180 KB)不再必然失败。**
- **Sources are now discovered from every known server, not one.** UDP source queries also went to the wrong port before, so that whole leg returned nothing. / **取源改为询问全部已知服务器**;此前 UDP 取源还发错了端口,那条通路一个源都拿不回来。
- **We no longer get silently banned by servers for asking too often**, and queue positions survive a dropped connection instead of sending you back to the end of the line. / **不再因请求过频被服务器静默封禁**,排队位置也不会因掉线而回到队尾。
- **Uploads no longer leak slots**, and when our turn comes in someone's queue the slot is actually taken. / **上传槽不再泄漏**;在别人队列里轮到我们时,槽位现在真的会被领走。
- **Download speed no longer reads 0 while data is arriving**, and progress no longer exceeds what is actually on disk. / **下载中速度不再显示为 0**,进度也不会超过磁盘上真实写入的量。
- **Disk write failures are now reported instead of silently mislabelled.** / **磁盘写入失败会如实报出**,不再被静默误判成其他原因。
- Setup now races sources in parallel and an unresponsive server no longer stalls the start of every download. / 启动阶段改为并行竞速探测源,单台服务器无响应不再拖住所有下载的启动。
- Each install now gets a persistent, properly marked eD2k UserHash. / 每次安装拥有持久且带标识的 eD2k UserHash。

### 📊 进度与状态显示 / Progress & Status

- Fix stalled-download false alarms: the detector now looks at bytes actually received on the wire, so a healthy slow download is no longer reported as stalled, and pausing or resuming no longer triggers a spurious warning. / 修复"停滞"误报:判据改用线上真实收到的字节数,健康的慢速下载不再被误判;暂停与恢复也不再触发一次假警报。
- ETA is computed from progress rate, and "receiving but cannot write to disk" is now surfaced as its own state. / ETA 改用进度速率计算;"在收但落不了盘"作为独立状态显示出来。
- Completed tasks show the file's total size instead of the bytes transferred in the last session. / 已完成任务显示文件总大小,而不是最后一轮的传输量。
- The connection count shows real connections, with queued sources counted separately; waiting caused by the anti-ban interval is shown as a deliberate wait rather than silence. / "连接数"显示真实连接数,排队数单独计;因防封禁间隔而等待会显示成一次刻意的等待,而不是毫无反馈。
- You can open the engine diagnostic log from the UI. / 可以从界面直接打开引擎诊断日志。

### 📦 安装包 / Installer

- **Fix an installer that installed successfully but produced an app that would not start**, when built with the Visual Studio generator. The packaging path did not account for the extra configuration subdirectory that multi-config generators add, so everything landed one level too deep while the shortcut pointed at the top level. CI builds (Ninja) were never affected. / **修复用 Visual Studio 生成器打出的安装包"安装成功却点不开"**:多配置生成器会在产物目录下多加一层配置子目录,打包路径没考虑到,内容全被装深了一层,而快捷方式指向顶层。CI 用的 Ninja 构建不受影响。
- Link-time artifacts (`.lib` / `.exp` / `.pdb`) and test binaries are no longer shipped in the installer. The exclusion rule existed but never took effect. / 安装包不再包含链接期产物(`.lib` / `.exp` / `.pdb`)与测试二进制 —— 排除规则本来就写了,但一直没生效。

### 📖 文档 / Docs

- Windows packaging instructions rewritten: the standard preset cannot be used for packaging, and the reason is now documented. / 重写 Windows 打包步骤:常用预设不能用于打包,原因已写明。

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
