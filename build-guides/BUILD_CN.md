# GDownload 构建指南

本指南详细说明如何在不同平台上从源代码构建 GDownload。

## 📋 目录

- [系统要求](#系统要求)
- [快速开始](#快速开始)
- [详细步骤](#详细步骤)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
- [使用 CMake Presets](#使用-cmake-presets)
- [构建选项](#构建选项)
- [常见问题](#常见问题)

---

## 系统要求

### 通用依赖

- **CMake** 3.18 或更高版本
- **Git** (用于克隆仓库和版本信息)
- **vcpkg** 包管理器
- **Qt 6.5.2** 或更高版本

### Windows

- **Visual Studio 2022** (建议使用 Community 版或更高)
- **MSVC 编译器** (C++20 支持)
- Qt 6.5.2+ (MSVC 2019 64-bit)

### macOS

- **Xcode Command Line Tools** 或完整 Xcode
- **Clang** 编译器
- **Ninja** 构建系统 (推荐)
  ```bash
  brew install ninja
  ```
- Qt 6.5.2+ (macOS universal)

### Linux

- **GCC 11+** 或 **Clang 14+**
- **Ninja** 构建系统 (推荐)
- Qt 6.5.2+ (gcc_64)
- 系统依赖包：
  ```bash
  # Ubuntu/Debian
  sudo apt-get install build-essential libgl1-mesa-dev libxkbcommon-x11-0 libxcb-xinerama0

  # Fedora/RHEL
  sudo dnf install gcc-c++ mesa-libGL-devel libxkbcommon-x11 xcb-util-renderutil
  ```

---

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/cool2528/GDownload.git
cd GDownload
git submodule update --init --recursive
```

### 2. 设置环境变量

<details>
<summary>Windows (PowerShell)</summary>

```powershell
$env:VCPKG_ROOT = "D:\tools\vcpkg"
$env:QTDIR = "C:\Qt\6.5.2\msvc2019_64"
```
</details>

<details>
<summary>macOS / Linux (Bash/Zsh)</summary>

```bash
export VCPKG_ROOT="/path/to/vcpkg"
export QTDIR="/path/to/Qt/6.5.2/macos"  # macOS
# export QTDIR="/path/to/Qt/6.5.2/gcc_64"  # Linux
```
</details>

### 3. 使用 CMake Presets 构建

```bash
# 配置项目
cmake --preset windows-msvc-user      # Windows
# cmake --preset osx-universal-user   # macOS Universal Binary
# cmake --preset ubuntu-amd64-user    # Linux

# 构建
cmake --build --preset windows-debug-user   # Windows
# cmake --build --preset osx-universal-debug-user   # macOS
# cmake --build --preset linux-debug-user          # Linux
```

---

## 详细步骤

### Windows

#### 前置准备

1. **安装 Visual Studio 2022**
   - 下载 [Visual Studio Community](https://visualstudio.microsoft.com/zh-hans/downloads/)
   - 安装时选择 "使用 C++ 的桌面开发" 工作负载

2. **安装 Qt**
   - 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - 安装 Qt 6.5.2，选择 "MSVC 2019 64-bit" 组件

3. **安装 vcpkg**
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git D:\tools\vcpkg
   cd D:\tools\vcpkg
   .\bootstrap-vcpkg.bat
   ```

#### 构建步骤

1. **使用 CMake Presets（推荐）**

   编辑 `CMakeUserPresets.json`，设置你的路径：
   ```json
   {
     "name": "windows-msvc-user",
     "environment": {
       "VCPKG_ROOT": "D:\\tools\\vcpkg",
       "QTDIR": "C:\\Qt\\6.5.2\\msvc2019_64"
     }
   }
   ```

   然后执行：
   ```powershell
   cmake --preset windows-msvc-user
   cmake --build --preset windows-debug-user
   ```

2. **手动配置（不使用 Presets）**

   ```powershell
   cmake -B build -S . `
     -G "Visual Studio 17 2022" -A x64 `
     -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
     -DCMAKE_PREFIX_PATH="$env:QTDIR" `
     -DCMAKE_BUILD_TYPE=Debug

   cmake --build build --config Debug --parallel
   ```

#### 生成安装包

```powershell
# 构建 Release 版本
cmake --preset windows-msvc-user
cmake --build build --config Release

# 使用 Inno Setup 生成安装程序
# 确保已安装 Inno Setup: choco install innosetup
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" build\release\windows_installer.iss
```

---

### macOS

#### 前置准备

1. **安装 Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **安装 Homebrew 和工具**
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew install cmake ninja create-dmg
   ```

3. **安装 Qt**
   - 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - 安装 Qt 6.5.2，选择 "macOS" 组件

4. **安装 vcpkg**
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   cd ~/vcpkg
   ./bootstrap-vcpkg.sh
   ```

#### 构建步骤

**选项 A：Universal Binary 分别构建（推荐，稳定可靠）**

这是生产环境推荐的方法，分别构建 ARM64 和 x86_64 版本后合并：

```bash
# 1. 构建 ARM64 版本
cmake --preset osx-arm64-user
cmake --build build-arm64 --config Release
cmake --install build-arm64

# 2. 构建 x86_64 版本
cmake --preset osx-x64-user
cmake --build build-x64 --config Release
cmake --install build-x64

# 3. 合并为 Universal Binary
./scripts/merge_macos_bundle.sh \
  install-arm64/GDownload.app \
  install-x64/GDownload.app \
  install-universal/GDownload.app

# 4. 验证
lipo -info install-universal/GDownload.app/Contents/MacOS/GDownload
# 应输出: Architectures in the fat file: ... are: x86_64 arm64
```

**选项 B：Universal Binary 一次性构建（不推荐，可能失败）**

使用 `osx-universal-user` preset 一次性构建：

```bash
cmake --preset osx-universal-user
cmake --build build-universal --config Release
```

> ⚠️ **注意**: 此方法依赖 vcpkg 的 Universal Binary 支持，某些依赖包可能构建失败。如遇到问题，请使用选项 A。

**选项 C：单一架构构建**

如果只需要特定架构：

```bash
# 仅 ARM64 (Apple Silicon)
cmake --preset osx-arm64-user
cmake --build build-arm64 --config Release

# 仅 x86_64 (Intel)
cmake --preset osx-x64-user
cmake --build build-x64 --config Release
```

#### 生成 DMG 安装包

```bash
cmake --build build --config Release
cmake --install build --config Release

# 使用 create-dmg 创建 DMG
create-dmg \
  --volname "GDownload" \
  --window-size 600 400 \
  --icon-size 100 \
  --app-drop-link 450 120 \
  GDownload.dmg \
  build/Release/bin/gdownload.app
```

---

### Linux

#### 前置准备

1. **安装系统依赖**

   <details>
   <summary>Ubuntu/Debian</summary>

   ```bash
   sudo apt-get update
   sudo apt-get install -y \
     build-essential cmake ninja-build git \
     libgl1-mesa-dev libxkbcommon-x11-0 libxcb-xinerama0 \
     libssl-dev libboost-dev
   ```
   </details>

   <details>
   <summary>Fedora/RHEL</summary>

   ```bash
   sudo dnf install -y \
     gcc-c++ cmake ninja-build git \
     mesa-libGL-devel libxkbcommon-x11 xcb-util-renderutil \
     openssl-devel boost-devel
   ```
   </details>

2. **安装 Qt**
   - 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - 安装 Qt 6.5.2，选择 "Desktop gcc 64-bit" 组件

3. **安装 vcpkg**
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   cd ~/vcpkg
   ./bootstrap-vcpkg.sh
   ```

#### 构建步骤

编辑 `CMakeUserPresets.json`：
```json
{
  "name": "ubuntu-amd64-user",
  "environment": {
    "VCPKG_ROOT": "/home/yourname/vcpkg",
    "QTDIR": "/home/yourname/Qt/6.5.2/gcc_64"
  }
}
```

构建：
```bash
cmake --preset ubuntu-amd64-user
cmake --build --preset linux-debug-user
```

#### 生成 AppImage

```bash
# 下载 linuxdeploy 工具
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

# 安装并打包
cmake --install build --config Release --prefix AppDir
export QMAKE=$QTDIR/bin/qmake
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
```

---

## 使用 CMake Presets

项目提供了预配置的 CMake Presets，简化构建流程。

### 可用的 Presets

#### Windows
- `windows-msvc-user` - Windows Debug 构建配置

#### macOS
- `osx-arm64-user` - macOS ARM64 (Apple Silicon)
- `osx-x64-user` - macOS x86_64 (Intel)
- `osx-universal-user` - **macOS Universal Binary** (用于本地测试，CI 推荐分别构建)

#### Linux
- `ubuntu-amd64-user` - Linux x64 Debug 构建

### 自定义 Presets

复制 `CMakeUserPresets.json.example` 为 `CMakeUserPresets.json`（如果不存在），然后修改环境变量：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "my-custom-preset",
      "inherits": "windows-msvc",  // 或 osx-universal, ubuntu-amd64
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo"
      },
      "environment": {
        "VCPKG_ROOT": "/your/vcpkg/path",
        "QTDIR": "/your/qt/path"
      }
    }
  ]
}
```

### Preset 命令

```bash
# 列出所有可用的 presets
cmake --list-presets

# 使用 preset 配置
cmake --preset <preset-name>

# 使用 build preset
cmake --build --preset <build-preset-name>
```

---

## 构建选项

### CMake 变量

| 变量 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `CMAKE_BUILD_TYPE` | STRING | `Debug` | 构建类型：Debug, Release, RelWithDebInfo |
| `CMAKE_PREFIX_PATH` | PATH | - | Qt 安装路径 |
| `CMAKE_TOOLCHAIN_FILE` | PATH | - | vcpkg toolchain 文件路径 |
| `CMAKE_OSX_ARCHITECTURES` | STRING | 自动检测 | macOS 架构：`arm64`, `x86_64`, 或 `arm64;x86_64` |
| `GDL_UI_GENERATE_TRANSLATIONS` | BOOL | `OFF` | 是否生成翻译文件 |

### vcpkg Triplets

项目使用自定义 triplets（位于 `custom-triplets/`）：

| 平台 | Triplet | 说明 |
|------|---------|------|
| Windows | `x64-windows` / `x64-windows-release` | x64 Windows |
| macOS | `arm64-osx` / `x64-osx` | 单一架构 |
| macOS | `universal-osx-release` | **Universal Binary** |
| Linux | `x64-linux` / `x64-linux-release` | x64 Linux |

### 构建类型对比

| 构建类型 | 优化 | 调试信息 | 用途 |
|----------|------|----------|------|
| Debug | ❌ | ✅ | 开发和调试 |
| Release | ✅ | ❌ | 生产发布 |
| RelWithDebInfo | ✅ | ✅ | 性能分析 |
| MinSizeRel | ✅ (体积) | ❌ | 最小体积发布 |

---

## 常见问题

### Q: vcpkg 首次构建时间很长

**A:** 这是正常的。vcpkg 需要从源代码编译所有依赖（Boost、OpenSSL、FramelessHelper 等），首次可能需要 30-60 分钟。后续构建会使用缓存。

**优化建议：**
- 使用 `--parallel` 参数并行构建
- 启用 vcpkg binary cache
- 使用 Release triplets（编译更快）

### Q: Qt 找不到

**错误示例：**
```
CMake Error: Could not find Qt6Config.cmake
```

**解决方案：**
1. 确保设置了 `QTDIR` 环境变量
2. 检查 `CMAKE_PREFIX_PATH` 是否正确指向 Qt 安装目录
3. 验证 Qt 安装完整性

```bash
# Windows
echo $env:QTDIR
# 应输出类似: C:\Qt\6.5.2\msvc2019_64

# macOS/Linux
echo $QTDIR
# 应输出类似: /Users/yourname/Qt/6.5.2/macos
```

### Q: macOS Universal Binary 构建失败

**可能原因：**
1. vcpkg 某些依赖不支持 Universal Binary 同时构建
2. Qt 版本不支持 Universal Binary（需要 Qt 6.2+）
3. 未正确指定 `CMAKE_OSX_ARCHITECTURES`

**解决方案：**

**推荐方法：分别构建后合并**
```bash
# 1. 分别构建 ARM64 和 x86_64
cmake --preset osx-arm64-user
cmake --build build-arm64 --config Release
cmake --install build-arm64

cmake --preset osx-x64-user
cmake --build build-x64 --config Release
cmake --install build-x64

# 2. 使用脚本合并
./scripts/merge_macos_bundle.sh \
  install-arm64/GDownload.app \
  install-x64/GDownload.app \
  install-universal/GDownload.app

# 3. 验证结果
lipo -info install-universal/GDownload.app/Contents/MacOS/GDownload
```

**验证 Qt 是否支持 Universal Binary：**
```bash
lipo -archs $QTDIR/lib/QtCore.framework/QtCore
# 应输出: arm64 x86_64
```

### Q: FramelessHelper 编译错误

**常见错误：**
```
FramelessHelper requires Qt 6.5 or later
```

**解决方案：**
1. 升级 Qt 到 6.5.2 或更高版本
2. 确保 `CMAKE_PREFIX_PATH` 正确
3. 检查 vcpkg triplet 配置

### Q: Windows 上 DLL 复制失败

**错误示例：**
```
Error copying file "...DLL..." to "...": command line too long
```

**解决方案：**
这个问题已在项目中修复（通过 `COMMAND_EXPAND_LISTS`）。如果仍遇到问题：
1. 拉取最新代码
2. 重新配置项目
3. 清理 build 目录后重新构建

### Q: 如何切换 Debug/Release 构建？

**方案 A：使用不同的 build 目录**
```bash
# Debug
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

**方案 B：多配置生成器（Visual Studio）**
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
cmake --build build --config Release
```

---

## 开发者工作流

### 推荐的开发流程

1. **配置一次**
   ```bash
   cmake --preset <your-platform>-user
   ```

2. **增量构建**
   ```bash
   cmake --build build --parallel
   ```

3. **运行应用**
   ```bash
   # Windows
   .\build\Debug\bin\Debug\gdownload.exe

   # macOS
   open build/Debug/bin/gdownload.app

   # Linux
   ./build/Debug/bin/gdownload
   ```

4. **清理构建**
   ```bash
   # 完全清理
   rm -rf build

   # 或只清理缓存
   cmake --build build --target clean
   ```

### IDE 集成

#### Visual Studio Code

安装 CMake Tools 扩展，然后：
1. `Ctrl+Shift+P` → "CMake: Select Configure Preset"
2. 选择对应平台的 preset
3. 按 `F7` 构建

#### CLion

1. Settings → Build, Execution, Deployment → CMake
2. 添加 Profile，使用 Preset
3. 使用内置构建工具

#### Qt Creator

1. File → Open File or Project → 选择 `CMakeLists.txt`
2. 在 Kit 配置中指定 Qt 路径
3. 构建 → 运行

---

## 参考资源

- [CMake 官方文档](https://cmake.org/documentation/)
- [vcpkg 官方文档](https://vcpkg.io/)
- [Qt 6 文档](https://doc.qt.io/qt-6/)
- [项目 Wiki](https://github.com/cool2528/GDownload/wiki)

---

## 获取帮助

如果遇到构建问题：

1. 查看 [GitHub Issues](https://github.com/cool2528/GDownload/issues)
2. 提交新的 Issue（附上完整的错误日志和环境信息）
3. 加入社区讨论

**提供信息模板：**
```
- 操作系统和版本：
- Qt 版本：
- CMake 版本：
- vcpkg 版本：
- 编译器版本：
- 完整错误日志：
```
