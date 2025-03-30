# GDownload

<p align="center">
  <img src="src/App/ui/Resource/images/logo/logo.svg" alt="GDownload Logo" width="128" height="128"/>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%20|%20macOS%20|%20Linux-lightgrey.svg" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/language-C++20-orange.svg" alt="Language"></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6.5+-green.svg" alt="Qt Version"></a>
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

GDownload 是一款现代化的跨平台下载管理器,使用 C++ 和 Qt 开发。它结合了现代技术栈和优秀的开源组件,为用户提供高效、稳定的下载体验。

## ✨ 特性

- 🖥️ 跨平台支持 (Windows, macOS, Linux)
- ⚡ 基于 aria2c 的高效下载引擎
- 🚀 多线程并发下载
- 🔄 支持多种协议 (HTTP, HTTPS, FTP, BitTorrent, Metalink)
- 📱 支持下载续传
- 🎨 美观的用户界面
- 🗂 支持百度网盘分享链接解析下载(只支持标准速度下载不支持加速下载，想高速下载请开通百度网盘官方SVIP)

## 🛠️ 技术栈

- 🎯 UI 框架: Qt Quick (QML) + Qt C++
- ⚙️ 核心引擎: aria2c
- 🌐 网络库: Boost.Asio
- 🔗 BT下载: LibtorrentRasterbar
- 📄 XML解析: PugiXML
- 🪟 无边框窗口: FramelessHelper

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

## 🚀 快速开始

1. 启动 GDownload
2. 输入要下载的文件URL
3. 选择保存位置
4. 点击"开始下载"

## 🤝 贡献

欢迎提交 Pull Request 或创建 Issue!

## 📄 开源协议

GDownload 使用 [Apache License 2.0](LICENSE.txt) 开源协议。

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
