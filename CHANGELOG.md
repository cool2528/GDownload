# 变更日志 / Changelog

## 2025-09-27 至 2025-11-09 版本更新

### ✨ 新增功能

#### 下载管理增强
- **删除任务确认功能** - 删除下载任务时可选择是否同时删除本地文件
  - 新增 `DeleteConfirmDialog` 组件用于确认删除操作
  - 支持可选地删除下载文件或仅删除任务记录
  - 增强 `BrowserManager` 的 `RemoveTask` 方法

#### 下载完成后自动化操作
- **任务事件自动执行** - 支持在任务开始、完成和出错时自动执行预定义动作
  - 播放提示音
  - 自动打开文件或目录
  - 运行自定义命令（支持变量替换：文件路径、目录、任务ID等）
  - 系统操作：关机、睡眠、重启
  - 新增设置页面用于配置这些行为

#### 浏览器插件支持
- **浏览器插件配置助手** - 新增浏览器插件配置助手卡片
  - 显示 WebSocket URL 和 RPC Secret
  - 一键复制配置信息
  - 状态指示器和使用说明
  - 支持 Chrome、Edge、Firefox 浏览器
  - 新增浏览器图标资源（chrome.svg、edge.svg、firefox.svg）

#### 高级下载设置
- **超时与重试设置** - 配置连接超时、传输超时、最大重试次数及重试间隔
- **BitTorrent 高级选项** - 启用 DHT、限制最大 Peer 数量、强制加密等
- **User-Agent 配置** - 提供多种浏览器 UA 预设并支持自定义
- **速度控制设置** - 全局下载/上传限速及最低速度限制配置
- **连接与性能参数** - 配置最大并发下载数、单服务器最大连接数、文件分片数及最小分割大小
- **Aria2 RPC 配置** - 支持自定义 RPC 端口与密钥，增强系统灵活性与安全性

#### UI 组件优化
- **关闭确认对话框** - 新增 `CloseConfirmDialog` 组件
  - 支持"最小化到托盘"、"直接退出"和"取消"选项
  - 可记住用户选择
  - 在设置中增加 `qShowCloseConfirm` 和 `qCloseToTray` 配置项
  
- **消息提示系统** - Element Plus 风格的消息提示
  - 新增 `GMessage`、`GMessageContainer` 组件
  - 支持多种类型、位置、持续时间、关闭按钮、Plain 样式
  - 提供 `GMessageTest` 测试页面验证功能
  
- **通用消息框组件** - 新增 `GMessageBox` 组件
  - 支持多种类型和自定义内容

### 🎨 UI/UX 改进

#### Element Plus 设计系统重构
- **浏览器模块 UI 重构** - 统一使用 Element Plus 设计规范
  - 优化高级设置页、基础设置页的整体布局和间距
  - 百度网盘设置卡片样式优化与描述增强
  - 下载页面标题栏和导航结构重新设计
  - 按钮、输入框、卡片等组件按照 Element Plus 标准更新
  - 增加设计变量（standardSpacing、cardSpacing 等）提升一致性

- **配色方案更新** - 更新 Element Plus 配色方案
  - 主色系调整为现代科技蓝
  - 优化暗色模式下的中性色彩对比度
  - 新增 `bgOverlay` 与 `bgElevated` 背景色定义，提升界面层次感
  - 采用 Ant Design 与 VS Code 主题色彩规范

#### 交互体验优化
- **下载项展示优化** - `GDownloadViewPage` 交互改进
  - 使用 hover 状态替代原有复杂选中逻辑
  - 调整文件名、进度条和状态信息区域的布局边距
  - 根据 pageType 控制部分文本可见性
  
- **任务对话框重构** - 拆分为独立组件
  - 新增 `TaskDialogHeader`、`TaskGeneralOptionsCard`、`TaskAdvancedOptionsCard`
  - 提升代码复用性和可维护性
  - 优化间距和高度计算逻辑，支持滚动区域自适应

### 🔧 核心功能优化

#### Aria2 集成增强
- **GitHub 代理域名随机选择** - 提高下载稳定性和可用性
  - 支持从多个预设代理域名中随机选择（ghfast.top、gh-proxy.com 等）
  
- **Tracker 列表同步优化** - 重构 `SyncMagnetServerList` 方法
  - 添加 ETag 缓存机制减少重复请求，提升性能
  - 支持 GitHub Raw 链接的 CDN 和代理降级策略
  - 引入 `TrackerETagCache` 数据库缓存实现
  - 增加 tracker 更新状态的通知与 UI 展示逻辑
  - 新增工具方法用于统计 tracker 数量及 URL 转换

#### 配置系统重构
- **配置文件格式迁移** - 从 INI 迁移到 TOML
  - 使用 tomlplusplus 库支持
  - 保留对旧版 INI 配置文件的自动迁移功能，确保向后兼容
  - 更新默认配置路径、会话路径处理
  
- **配置键类型安全** - 将硬编码字符串键替换为 `CONFIG_KEY_PATH` 宏定义
  - 提高类型安全性和可维护性
  - 使用 `config::Keys` 统一管理配置键

### 🧪 测试与质量保证

#### 测试框架完善
- **完整的单元测试和集成测试框架** - 基于 Google Test
  - 新增 CMake 配置支持 `BUILD_TESTS` 选项
  - 实现 `UtilsToolsManager` 的 `RelaunchAfterExit` 功能全面测试
  - 支持跨平台的进程重启逻辑测试（Windows、macOS、Linux）
  - 增加测试文档 README.md 说明测试结构、运行方法及编写指南
  - 提供覆盖率检查与测试目标清理等辅助构建目标
  - 修复 RelaunchAfterExit 在 Windows 上的路径转义和多方案回退机制

### 🛠️ 构建与部署

#### CI/CD 优化
- **多平台工作流合并** - 统一整合 CI 工作流
  - 将 MacOSX.yml、linux.yml 和 windows.yml 合并为 cli-matrix.yml
  - 支持 Linux、macOS 和 Windows 三大平台并行构建与发布
  - 提高 CI 效率与维护性

#### CMake 配置优化
- **翻译文件生成可选配置** - 新增 `GDL_UI_GENERATE_TRANSLATIONS` 选项
  - 控制是否在 configure 阶段生成 TS/QM 翻译文件
  - 默认关闭以减少不必要的构建依赖和时间
  - 优化 Qt LinguistTools 的查找逻辑

### 🌍 国际化

#### 翻译更新
- **日语翻译完善** - 完善 gdownload_ja_JP.ts 翻译
  - 移除部分未完成标记(type="unfinished")
  - 提升本地化质量
  
- **图标更新** - 更新部分界面图标
  - HelpDialog 中 "License" 图标从 FileText 更改为 Code
  - TaskDialogPage 中 "Baidu" 图标从 CloudFolder 更改为 Cloud

### 📦 依赖管理

#### 新增依赖
- **tomlplusplus** - 支持 TOML 配置文件格式
- 在 vcpkg.json 中添加并优化依赖顺序

### 🐛 Bug 修复

- 修复 Windows 平台上 `RelaunchAfterExit` 的路径转义问题
- 修正 `GCard` 的 padding 计算方式，避免重复累加
- 优化多处 UI 布局和间距问题

---

## 技术亮点

- **架构优化**：模块化设计，组件拆分提升可维护性
- **用户体验**：遵循 Element Plus 设计规范，提供一致的现代化界面
- **性能优化**：引入缓存机制，减少不必要的网络请求
- **平台兼容**：完善跨平台支持和测试覆盖
- **开发者友好**：完善测试框架和文档，提升开发效率

---

**发布日期**: 2025-11-09  
**提交数量**: 15+ commits  
**涉及文件**: 100+ 文件变更
