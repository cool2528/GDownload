# Changelog

## v1.0.6

### 🚀 Major Updates / 重大更新

**Qt Framework Upgrade**<br>
**Qt 框架升级**

- Upgraded to Qt 6.10.1 for all platforms<br>
全平台升级至 Qt 6.10.1
- Added multiple compatibility patches for framelesshelper on Qt 6.10<br>
为 framelesshelper 添加多个 Qt 6.10 兼容性补丁
- Fixed darkmode detection, compositing, constexpr, and size comparison issues<br>
修复深色模式检测、合成渲染、constexpr 和尺寸比较问题
- Resolved quickcontrols2 private component access issues<br>
解决 quickcontrols2 私有组件访问问题

**macOS Universal Binary Support**<br>
**macOS Universal Binary 完整支持**

- Complete support for macOS Universal Binary builds (ARM64 + x86_64)<br>
完整支持 macOS Universal Binary 构建（ARM64 + x86_64）
- Added separate build and merge workflow for ARM64 and x86_64 architectures<br>
新增 ARM64 和 x86_64 架构的分别构建与合并工作流
- New merge script for combining architecture-specific builds<br>
新增合并脚本用于组合特定架构的构建产物
- Removed deprecated AGL framework linking on macOS<br>
移除 macOS 上已弃用的 AGL 框架链接
- Enhanced CMake presets with independent ARM64, x86_64, and Universal Binary configurations<br>
增强 CMake presets，提供独立的 ARM64、x86_64 和 Universal Binary 配置

**Windows Installer Modernization**<br>
**Windows 安装程序现代化**

- Migrated from NSIS to Inno Setup for a modern installation experience<br>
从 NSIS 迁移至 Inno Setup，提供现代化安装体验
- Support for system-wide and user-level installation options<br>
支持系统级和用户级安装选项
- Automatic PATH environment variable configuration<br>
自动配置 PATH 环境变量
- .torrent file association support<br>
.torrent 文件关联支持

### ✨ New Features / 新增功能

**GitHub Download Acceleration**<br>
**GitHub 下载加速**

- Added "Enable GitHub Accelerated Downloads" toggle in settings<br>
设置中新增"启用 GitHub 加速下载"开关
- Optional ghproxy mirror for faster update downloads (disabled by default)<br>
可选使用 ghproxy 镜像加速更新下载（默认关闭）
- Improved update flow with better state handling and error callbacks<br>
优化更新流程的状态处理和错误回调

**macOS System Tray Enhancement**<br>
**macOS 系统托盘增强**

- Added new tray icon templates (minimal, brand, Apple-style variants)<br>
新增托盘图标模板（简约、品牌、Apple 风格）
- Improved icon visibility across different macOS themes<br>
改善不同 macOS 主题下的图标可见性
- Updated resource files with new icon assets<br>
更新资源文件，包含新图标资源

**Aria2 Configuration**<br>
**Aria2 配置**

- Added dedicated Aria2 configuration files for better engine control<br>
添加专用 Aria2 配置文件，更好地控制引擎
- Reorganized engine binaries directory structure (darwin/x86_64)<br>
重组引擎二进制文件目录结构（darwin/x86_64）

### 📚 Documentation / 文档改进

**Build Guides**<br>
**构建指南**

- Comprehensive build documentation for all platforms<br>
全平台详细构建文档
- Detailed system requirements and dependency installation guides<br>
系统要求和依赖安装详细指南
- Quick start with CMake Presets<br>
使用 CMake Presets 快速开始
- Platform-specific detailed instructions (Windows, macOS, Linux)<br>
平台特定的详细说明（Windows、macOS、Linux）
- Build options configuration<br>
构建选项配置
- Troubleshooting solutions<br>
故障排查方案
- Recommended developer workflow<br>
推荐的开发者工作流

**README Enhancements**<br>
**README 增强**

- Added "Building from Source" section with links to detailed guides<br>
添加"从源代码构建"章节及详细指南链接
- Updated Qt version badges to 6.10.1+<br>
更新 Qt 版本徽章为 6.10.1+

### 🌍 Localization / 本地化

- Enriched translation files with GDownload introduction information<br>
丰富翻译文件，添加 GDownload 介绍信息
- Updated Japanese (ja_JP), Korean (ko_KR), Simplified Chinese (zh_CN), and Traditional Chinese (zh_TW) translations<br>
更新日语（ja_JP）、韩语（ko_KR）、简体中文（zh_CN）和繁体中文（zh_TW）翻译

### 🔧 Build System Improvements / 构建系统改进

**vcpkg Configuration**<br>
**vcpkg 配置**

- Added QTDIR environment variable support to all custom triplets<br>
所有自定义 triplets 添加 QTDIR 环境变量支持
- New universal-osx-release triplet for Universal Binary builds<br>
新增 universal-osx-release triplet 用于 Universal Binary 构建
- Updated framelesshelper port files with multiple Qt 6.10 compatibility patches<br>
更新 framelesshelper 端口文件，包含多个 Qt 6.10 兼容性补丁
- Added vcpkg overlay ports configuration for framelesshelper builds<br>
添加 vcpkg overlay ports 配置用于 framelesshelper 构建

**CMake Enhancements**<br>
**CMake 增强**

- Optimized architecture detection logic (auto-detect or manual specification)<br>
优化架构检测逻辑（自动检测或手动指定）
- Improved build configuration for different architectures<br>
改进不同架构的构建配置
- Enhanced preset system for easier cross-platform development<br>
增强 preset 系统，简化跨平台开发

**CI/CD Workflow**<br>
**CI/CD 工作流**

- Consolidated multi-platform GitHub Actions workflow<br>
整合多平台 GitHub Actions 工作流
- Separate ARM64 and x86_64 builds for macOS with automatic merging<br>
macOS 分别构建 ARM64 和 x86_64，自动合并
- Streamlined installation package generation process<br>
简化安装包生成流程
- Updated release notes preparation with CHANGELOG.md integration<br>
更新 release notes 准备流程，集成 CHANGELOG.md

### 🐛 Bug Fixes / Bug 修复

- Fixed Windows automatic update issues<br>
修复 Windows 自动更新问题
- Fixed GTK settings initialization on Linux<br>
修复 Linux 上的 GTK 设置初始化问题
- Corrected framelesshelper compatibility issues with Qt 6.10<br>
修正 framelesshelper 与 Qt 6.10 的兼容性问题
- Resolved build system path issues for macOS engine binaries<br>
解决 macOS 引擎二进制文件的构建系统路径问题
- Fixed various Qt 6.10 API compatibility warnings<br>
修复各种 Qt 6.10 API 兼容性警告

### 🛠️ Technical Details / 技术细节

**Modified Components**<br>
**修改组件统计**

- 12 files changed for macOS tray icon support<br>
12 个文件改动用于 macOS 托盘图标支持
- 7 files changed for Windows installer migration<br>
7 个文件改动用于 Windows 安装程序迁移
- 8 files changed for macOS build workflow improvements<br>
8 个文件改动用于 macOS 构建工作流改进
- Multiple patches added for Qt 6.10 compatibility<br>
为 Qt 6.10 兼容性添加多个补丁

**Framework Updates**<br>
**框架更新**

- Updated framelesshelper with GTK settings fix<br>
更新 framelesshelper，包含 GTK 设置修复
- Added fix-qt-6.10-compatibility.patch<br>
添加 fix-qt-6.10-compatibility.patch
- Added fix-qt-6.10-compositing.patch<br>
添加 fix-qt-6.10-compositing.patch
- Added fix-qt-6.10-constexpr.patch<br>
添加 fix-qt-6.10-constexpr.patch
- Added fix-qt-6.10-darkmode.patch<br>
添加 fix-qt-6.10-darkmode.patch
- Added fix-quickcontrols2-private.patch<br>
添加 fix-quickcontrols2-private.patch
- Added remove-agl-framework.patch<br>
添加 remove-agl-framework.patch

---

**Full Changelog**: https://github.com/cool2528/GDownload/compare/v1.0.5...v1.0.6<br>
**完整变更日志**: https://github.com/cool2528/GDownload/compare/v1.0.5...v1.0.6
