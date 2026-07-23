# GDownload

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="src/App/ui/Resource/images/logo/aurora-brand-dark.svg">
    <img src="src/App/ui/Resource/images/logo/aurora-brand-light.svg" alt="GDownload Logo" width="128" height="128"/>
  </picture>
</p>

<p align="center">
  <a href="LICENSE.txt"><img src="https://img.shields.io/badge/license-AGPL%20v3-blue.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%20|%20macOS%20|%20Linux-lightgrey.svg" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/language-C++20-orange.svg" alt="Language"></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6.10.1+-green.svg" alt="Qt Version"></a>
</p>

<p align="center">
  <a href="https://github.com/cool2528/GDownload/stargazers">
    <img src="https://img.shields.io/github/stars/cool2528/GDownload?style=social" alt="Stars">
  </a>
  <a href="https://github.com/cool2528/GDownload/network/members">
    <img src="https://img.shields.io/github/forks/cool2528/GDownload?style=social" alt="Forks">
  </a>
</p>

[English](README_EN.md) | 简体中文

GDownload 是一款现代化的跨平台下载管理器，使用 C++ 和 Qt 开发。它结合了现代技术栈和优秀的开源组件，为用户提供高效、稳定的下载体验。

## 📸 界面展示

<p align="center">
  <img src="screenshot/screenshot-1.png" alt="Light Theme" width="45%"/>
  <img src="screenshot/screenshot-2.png" alt="Dark Theme" width="45%"/>
</p>
<p align="center"><i>浅色主题与深色主题</i></p>

## ✨ 核心特性

### 🚀 强大的下载引擎
- ⚡ 基于 aria2c 的高性能下载引擎
- 🔄 支持多种协议 (HTTP/HTTPS/FTP/BitTorrent/Metalink)
- 🚄 多线程并发下载，最大化带宽利用
- 📱 断点续传，随时暂停和恢复
- 🔗 磁力链接和种子文件支持

### 🔗 eD2k / eMule 网络
- 🐘 完整的 eD2k / eMule 客户端 —— 下载、搜索、服务器管理、文件分享与 Kad DHT
- 📥 支持 `ed2k://` 链接下载,多源流水线、上传队列排队等待、断点续传、Kad 源发现
- 🔍 在 eD2k 服务器与 Kad 网络中搜索文件
- 🖥️ 服务器管理(连接 / 增删 / 更新 `server.met` / Kad 状态)
- 📤 文件分享,含上传统计与队列管理
- ⚙️ 由自研的进程内 eD2k 引擎驱动 —— [**ed2k-engine-cpp**](https://github.com/cool2528/ed2k-engine-cpp)

### 🎨 现代化界面
- 🖥️ 跨平台支持 (Windows、macOS、Linux)
- 🌓 浅色/深色主题自动切换
- 🎯 Aurora UI 设计系统
- 🌍 多语言支持 (简体中文、繁体中文、English、日本語、한국어)
- 💫 流畅的动画和交互体验

### ⚙️ 高级功能

#### 速度控制
- 📊 全局下载速度限制
- 📈 全局上传速度限制（BT/种子）
- 🎚️ 实时速度调整

#### 连接与性能优化
- 🔌 单服务器最大连接数配置
- 🔢 最小分段大小设置
- 🧩 文件分段数量控制
- 📦 磁盘缓存大小优化
- 🔄 最大同时下载任务数

#### BitTorrent 增强
- 🌐 BT Tracker 服务器自动同步
- 📡 自定义 Tracker 源
- ⏱️ DHT 超时配置
- 🔁 超时自动重试

#### 下载完成后操作
- 🔔 系统通知提醒
- 🎵 自定义提示音
- 🔊 语音播报
- 📂 自动打开下载文件夹
- 💻 运行自定义命令
- 🔌 电脑自动关机/休眠/睡眠

### 🗂️ 网盘集成
- 📦 百度网盘分享链接解析下载
  - ⚠️ 仅支持标准速度下载（非加速下载）
  - 💡 高速下载请开通百度网盘官方 SVIP
- 🔌 插件化架构，易于扩展其他网盘

### 🧩 浏览器插件 (实验功能)

> 💡 **提升下载效率**: 直接从浏览器捕获下载链接，无需复制粘贴

#### 支持的浏览器
- 🌐 **Chrome** / **Edge** / **Firefox**

#### 核心功能
- 🔗 **一键捕获**: 自动检测网页中的下载链接
- 📋 **批量下载**: 选择多个链接一次性发送到 GDownload
- 🎨 **统一设计**: 完美匹配 GDownload 的 Aurora UI 界面风格
- 🔒 **安全连接**: 通过 WebSocket 直连 aria2c，无需额外服务
- 🎯 **智能过滤**: 按文件大小、类型、自定义规则筛选链接
- ⚡ **开箱即用**: 默认配置已优化，无需额外设置

#### 快速安装

**步骤 1: 下载插件**
- 访问插件仓库: [gd-browser-extension](https://github.com/cool2528/gd-browser-extension/releases)
- 或通过 GDownload 应用内引导: `设置` → `Lab` → `浏览器插件`

**步骤 2: 安装到浏览器**

<details>
<summary>Chrome / Edge 安装方法</summary>

1. 打开扩展管理页面:
   - Chrome: `chrome://extensions/`
   - Edge: `edge://extensions/`
2. 开启右上角的 **"开发者模式"**
3. 点击 **"加载已解压的扩展程序"**
4. 选择解压后的 `dist` 文件夹
</details>

<details>
<summary>Firefox 安装方法</summary>

1. 打开 `about:debugging#/runtime/this-firefox`
2. 点击 **"加载临时附加组件"**
3. 选择 `dist` 文件夹中的 `manifest.json`
</details>

**步骤 3: 开始使用**
- ✅ 插件会自动连接到 GDownload
- ✅ 浏览网页时，点击插件图标即可捕获下载链接
- ✅ 选择链接后一键发送到 GDownload 开始下载

> 📖 **详细文档**: 应用内前往 `设置` → `Lab` 查看完整安装指南和配置说明

## 🔌 插件市场

GDownload 采用 JS 插件架构，网盘解析功能均以插件形式提供，支持沙箱运行和热更新。

### 📖 插件使用教程

- [百度网盘插件配置指南](docs/plugin-guides/baidu-netdisk.md)
- [夸克网盘插件配置指南](docs/plugin-guides/quark-netdisk.md)
- [阿里云盘插件配置指南](docs/plugin-guides/aliyundrive-netdisk.md)
- [123 云盘插件配置指南](docs/plugin-guides/pan123-netdisk.md)

### 🔧 插件开发

想为 GDownload 开发网盘插件？请阅读 [JS 插件系统设计文档](docs/JS_PLUGIN_SYSTEM_DESIGN.md)，了解插件架构、SDK API 和开发规范。

## 🛠️ 技术栈

- 🎯 **UI 框架**: Qt Quick (QML) + Qt C++
- ⚙️ **核心引擎**: aria2c
- 🌐 **网络库**: Boost.Asio + Beast (WebSocket)
- 🔗 **BT 下载**: LibtorrentRasterbar
- 🐘 **eD2k / eMule**: 自研引擎 —— [ed2k-engine-cpp](https://github.com/cool2528/ed2k-engine-cpp)
- 💾 **数据存储**: SQLite3
- 📝 **日志系统**: spdlog
- 📄 **数据解析**: nlohmann-json, PugiXML
- 🪟 **无边框窗口**: FramelessHelper
- 🏗️ **构建系统**: CMake + vcpkg

## 📦 安装

[Releases](https://github.com/cool2528/GDownload/releases) 页面下载最新版本

### macOS 常见问题

在 macOS 系统中，如果出现"文件已损坏"或"无法打开应用程序"的提示，这是因为应用程序没有开发者签名所导致的安全限制，可通过以下步骤解决：

1. 打开"系统偏好设置" > "安全性与隐私" > "通用"，点击"仍要打开"按钮（如果显示）
2. 如果上述方法无效，请打开终端(Terminal)，输入以下命令：
   ```
   sudo xattr -r -d com.apple.quarantine /Applications/GDownload.app
   ```
   注意：请将路径替换为您实际安装的位置

3. 输入管理员密码后，再次尝试打开应用程序

## 🔨 从源代码构建

如果您想从源代码编译 GDownload，我们为开发者提供了详细的构建指南。

👉 **[查看完整构建文档](build-guides/BUILD_CN.md)**

该文档包含：
- 📋 系统要求和依赖安装
- 🚀 使用 CMake Presets 快速构建
- 🔧 平台特定的详细步骤（Windows、macOS、Linux）
- ⚙️ 构建选项配置
- 🐛 常见问题解决方案
- 💡 开发者工作流推荐

## 🚀 快速开始

1. 从 [Releases](https://github.com/cool2528/GDownload/releases) 下载适合你系统的安装包
2. 启动 GDownload
3. 添加下载任务：
   - 点击 ➕ 按钮或使用快捷键 `Ctrl+N`
   - 输入下载链接（支持 HTTP/HTTPS/FTP/磁力链接/种子文件）
   - 选择保存位置
   - 配置下载选项（可选）
4. 点击"开始下载"
5. 在设置中自定义你的下载体验

### 💡 使用技巧

- **批量下载**: 在新建任务对话框中，每行输入一个链接
- **剪贴板监听**: 启用后自动捕获复制的下载链接
- **🔥 浏览器插件**: 前往 `设置` → `Lab`，安装浏览器插件享受更便捷的下载体验
  - 无需复制链接，直接从网页捕获
  - 支持批量选择和智能过滤
  - 一键发送到 GDownload
- **自定义 User-Agent**: 高级设置中可配置 HTTP 请求头，绕过某些网站的下载限制
- **BT 加速**: 设置中可添加和更新 Tracker 服务器列表，提升 BT 下载速度

## 🤝 贡献

欢迎提交 Pull Request 或创建 Issue!

## 📄 开源协议

GDownload 使用 [GNU Affero General Public License v3.0](LICENSE.txt)（AGPL-3.0）开源协议。

- ✅ 可以自由使用、修改、分发本软件（包括商业性分发）
- ⚠️ 分发修改版，或将本软件（及其修改版）作为网络服务对外提供时，必须以相同协议开放完整源代码
- ⚠️ 必须保留原有版权声明和协议文本

如需在 AGPL-3.0 条款之外使用本项目（例如闭源集成、不希望公开服务端源码的商业场景），请通过 [Issues](https://github.com/cool2528/GDownload/issues) 或邮箱 252858527@qq.com 联系作者洽谈商业授权。

> 说明：v1.0.8 及更早的历史发布版本仍适用其发布时随附的 Apache License 2.0；自此之后的版本适用 AGPL-3.0。

### 第三方组件

本项目使用了多个优秀的开源组件,包括:

- Qt Framework (LGPL v3)
- FramelessHelper (MIT)
- Boost Libraries (Boost Software License)
- LibtorrentRasterbar (BSD)
- PugiXML (MIT)

详细的第三方组件信息和许可证声明请查看 [NOTICE](NOTICE) 文件。

## 🌟 鸣谢

感谢所有为该项目做出贡献的开发者和用户！

## 📱 联系我们

- [GitHub](https://github.com/cool2528/GDownload)
- [问题追踪](https://github.com/cool2528/GDownload/issues)

## ⚠️ 免责声明

GDownload 仅作为下载工具，供用户合法地下载互联网资源，使用过程中请遵守当地法律法规。

- 本软件不会收集任何用户隐私信息
- 用户使用本软件下载的所有资源版权归原作者或其合法持有人所有
- 开发者不对用户使用本软件下载的内容负责，也不对因使用本软件可能导致的任何损失或损害承担责任
- 本软件解析百度网盘分享链接的功能仅用于合法获取用户自己的文件，不得用于侵犯他人知识产权
- 如有任何功能违反相关法律法规，请及时通过 Issues 联系我们，我们将立即处理

使用本软件即表示您已阅读并同意本免责声明的所有条款。
