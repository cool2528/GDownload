# GDownload Build Guide

This guide provides detailed instructions for building GDownload from source on different platforms.

## 📋 Table of Contents

- [System Requirements](#system-requirements)
- [Quick Start](#quick-start)
- [Detailed Instructions](#detailed-instructions)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
- [Using CMake Presets](#using-cmake-presets)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

### Common Dependencies

- **CMake** 3.18 or later
- **Git** (for cloning repository and version information)
- **vcpkg** package manager
- **Qt 6.5.2** or later

### Windows

- **Visual Studio 2022** (Community edition or higher recommended)
- **MSVC Compiler** (C++20 support required)
- Qt 6.5.2+ (MSVC 2019 64-bit)

### macOS

- **Xcode Command Line Tools** or full Xcode
- **Clang** compiler
- **Ninja** build system (recommended)
  ```bash
  brew install ninja
  ```
- Qt 6.5.2+ (macOS universal)

### Linux

- **GCC 11+** or **Clang 14+**
- **Ninja** build system (recommended)
- Qt 6.5.2+ (gcc_64)
- System dependencies:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install build-essential libgl1-mesa-dev libxkbcommon-x11-0 libxcb-xinerama0

  # Fedora/RHEL
  sudo dnf install gcc-c++ mesa-libGL-devel libxkbcommon-x11 xcb-util-renderutil
  ```

---

## Quick Start

### 1. Clone Repository

```bash
git clone https://github.com/cool2528/GDownload.git
cd GDownload
git submodule update --init --recursive
```

### 2. Set Environment Variables

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

### 3. Build Using CMake Presets

```bash
# Configure
cmake --preset windows-msvc-user      # Windows
# cmake --preset osx-universal-user   # macOS Universal Binary
# cmake --preset ubuntu-amd64-user    # Linux

# Build
cmake --build --preset windows-debug-user   # Windows
# cmake --build --preset osx-universal-debug-user   # macOS
# cmake --build --preset linux-debug-user          # Linux
```

---

## Detailed Instructions

### Windows

#### Prerequisites

1. **Install Visual Studio 2022**
   - Download [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
   - Select "Desktop development with C++" workload during installation

2. **Install Qt**
   - Download [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - Install Qt 6.5.2, select "MSVC 2019 64-bit" component

3. **Install vcpkg**
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git D:\tools\vcpkg
   cd D:\tools\vcpkg
   .\bootstrap-vcpkg.bat
   ```

#### Build Steps

1. **Using CMake Presets (Recommended)**

   Edit `CMakeUserPresets.json` with your paths:
   ```json
   {
     "name": "windows-msvc-user",
     "environment": {
       "VCPKG_ROOT": "D:\\tools\\vcpkg",
       "QTDIR": "C:\\Qt\\6.5.2\\msvc2019_64"
     }
   }
   ```

   Then execute:
   ```powershell
   cmake --preset windows-msvc-user
   cmake --build --preset windows-debug-user
   ```

2. **Manual Configuration (Without Presets)**

   ```powershell
   cmake -B build -S . `
     -G "Visual Studio 17 2022" -A x64 `
     -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
     -DCMAKE_PREFIX_PATH="$env:QTDIR" `
     -DCMAKE_BUILD_TYPE=Debug

   cmake --build build --config Debug --parallel
   ```

#### Creating Installer

```powershell
# Build Release version
cmake --preset windows-msvc-user
cmake --build build --config Release

# Generate installer with NSIS
makensis build\release\windows_installer.nsi
```

---

### macOS

#### Prerequisites

1. **Install Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Install Homebrew and Tools**
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew install cmake ninja create-dmg
   ```

3. **Install Qt**
   - Download [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - Install Qt 6.5.2, select "macOS" component

4. **Install vcpkg**
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   cd ~/vcpkg
   ./bootstrap-vcpkg.sh
   ```

#### Build Steps

**Option A: Universal Binary (Recommended - supports both Intel and Apple Silicon)**

Edit `CMakeUserPresets.json`:
```json
{
  "name": "osx-universal-user",
  "environment": {
    "VCPKG_ROOT": "/Users/yourname/vcpkg",
    "QTDIR": "/Users/yourname/Qt/6.5.2/macos"
  }
}
```

Build:
```bash
cmake --preset osx-universal-user
cmake --build --preset osx-universal-debug-user
```

Verify Universal Binary:
```bash
lipo -archs build/Debug/bin/gdownload.app/Contents/MacOS/gdownload
# Should output: arm64 x86_64
```

**Option B: Single Architecture**

```bash
# ARM64 only (Apple Silicon)
cmake --preset osx-clang-user
cmake --build --preset osx-debug-user

# Or manually specify architecture
cmake -B build -S . \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_PREFIX_PATH="$QTDIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_ARCHITECTURES="arm64"  # or "x86_64"
```

#### Creating DMG Package

```bash
cmake --build build --config Release
cmake --install build --config Release

# Create DMG with create-dmg
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

#### Prerequisites

1. **Install System Dependencies**

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

2. **Install Qt**
   - Download [Qt Online Installer](https://www.qt.io/download-qt-installer)
   - Install Qt 6.5.2, select "Desktop gcc 64-bit" component

3. **Install vcpkg**
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   cd ~/vcpkg
   ./bootstrap-vcpkg.sh
   ```

#### Build Steps

Edit `CMakeUserPresets.json`:
```json
{
  "name": "ubuntu-amd64-user",
  "environment": {
    "VCPKG_ROOT": "/home/yourname/vcpkg",
    "QTDIR": "/home/yourname/Qt/6.5.2/gcc_64"
  }
}
```

Build:
```bash
cmake --preset ubuntu-amd64-user
cmake --build --preset linux-debug-user
```

#### Creating AppImage

```bash
# Download linuxdeploy tools
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

# Install and package
cmake --install build --config Release --prefix AppDir
export QMAKE=$QTDIR/bin/qmake
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
```

---

## Using CMake Presets

The project provides pre-configured CMake Presets to simplify the build process.

### Available Presets

#### Windows
- `windows-msvc-user` - Windows Debug build configuration

#### macOS
- `osx-clang-user` - macOS ARM64 Debug build
- `osx-universal-user` - **macOS Universal Binary** (ARM64 + x86_64)

#### Linux
- `ubuntu-amd64-user` - Linux x64 Debug build

### Customizing Presets

Copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` (if it doesn't exist), then modify environment variables:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "my-custom-preset",
      "inherits": "windows-msvc",  // or osx-universal, ubuntu-amd64
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

### Preset Commands

```bash
# List all available presets
cmake --list-presets

# Configure using preset
cmake --preset <preset-name>

# Build using build preset
cmake --build --preset <build-preset-name>
```

---

## Build Options

### CMake Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `CMAKE_BUILD_TYPE` | STRING | `Debug` | Build type: Debug, Release, RelWithDebInfo |
| `CMAKE_PREFIX_PATH` | PATH | - | Qt installation path |
| `CMAKE_TOOLCHAIN_FILE` | PATH | - | vcpkg toolchain file path |
| `CMAKE_OSX_ARCHITECTURES` | STRING | Auto-detect | macOS architectures: `arm64`, `x86_64`, or `arm64;x86_64` |
| `GDL_UI_GENERATE_TRANSLATIONS` | BOOL | `OFF` | Whether to generate translation files |

### vcpkg Triplets

The project uses custom triplets (located in `custom-triplets/`):

| Platform | Triplet | Description |
|----------|---------|-------------|
| Windows | `x64-windows` / `x64-windows-release` | x64 Windows |
| macOS | `arm64-osx` / `x64-osx` | Single architecture |
| macOS | `universal-osx-release` | **Universal Binary** |
| Linux | `x64-linux` / `x64-linux-release` | x64 Linux |

### Build Type Comparison

| Build Type | Optimization | Debug Info | Use Case |
|------------|-------------|------------|----------|
| Debug | ❌ | ✅ | Development and debugging |
| Release | ✅ | ❌ | Production release |
| RelWithDebInfo | ✅ | ✅ | Performance profiling |
| MinSizeRel | ✅ (size) | ❌ | Minimal size release |

---

## Troubleshooting

### Q: First vcpkg build takes a long time

**A:** This is normal. vcpkg compiles all dependencies from source (Boost, OpenSSL, FramelessHelper, etc.), which may take 30-60 minutes on first build. Subsequent builds use cache.

**Optimization tips:**
- Use `--parallel` flag for parallel builds
- Enable vcpkg binary cache
- Use Release triplets (faster compilation)

### Q: Qt not found

**Error example:**
```
CMake Error: Could not find Qt6Config.cmake
```

**Solution:**
1. Ensure `QTDIR` environment variable is set
2. Check `CMAKE_PREFIX_PATH` points to Qt installation directory
3. Verify Qt installation integrity

```bash
# Windows
echo $env:QTDIR
# Should output something like: C:\Qt\6.5.2\msvc2019_64

# macOS/Linux
echo $QTDIR
# Should output something like: /Users/yourname/Qt/6.5.2/macos
```

### Q: macOS Universal Binary build fails

**Possible causes:**
1. Qt version doesn't support Universal Binary (requires Qt 6.2+)
2. vcpkg dependencies not properly built as Universal Binary
3. `CMAKE_OSX_ARCHITECTURES` not specified

**Solution:**
```bash
# Explicitly specify architectures
cmake --preset osx-universal-user \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# Verify Qt supports Universal Binary
lipo -archs $QTDIR/lib/QtCore.framework/QtCore
# Should output: arm64 x86_64
```

### Q: FramelessHelper compilation error

**Common error:**
```
FramelessHelper requires Qt 6.5 or later
```

**Solution:**
1. Upgrade Qt to 6.5.2 or later
2. Ensure `CMAKE_PREFIX_PATH` is correct
3. Check vcpkg triplet configuration

### Q: Windows DLL copy failure

**Error example:**
```
Error copying file "...DLL..." to "...": command line too long
```

**Solution:**
This issue has been fixed in the project (via `COMMAND_EXPAND_LISTS`). If you still encounter it:
1. Pull latest code
2. Reconfigure project
3. Clean build directory and rebuild

### Q: How to switch between Debug/Release builds?

**Option A: Use different build directories**
```bash
# Debug
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# Release
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

**Option B: Multi-config generator (Visual Studio)**
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
cmake --build build --config Release
```

---

## Developer Workflow

### Recommended Development Flow

1. **Configure once**
   ```bash
   cmake --preset <your-platform>-user
   ```

2. **Incremental builds**
   ```bash
   cmake --build build --parallel
   ```

3. **Run application**
   ```bash
   # Windows
   .\build\Debug\bin\Debug\gdownload.exe

   # macOS
   open build/Debug/bin/gdownload.app

   # Linux
   ./build/Debug/bin/gdownload
   ```

4. **Clean builds**
   ```bash
   # Complete clean
   rm -rf build

   # Or just clean cache
   cmake --build build --target clean
   ```

### IDE Integration

#### Visual Studio Code

Install CMake Tools extension, then:
1. `Ctrl+Shift+P` → "CMake: Select Configure Preset"
2. Select your platform's preset
3. Press `F7` to build

#### CLion

1. Settings → Build, Execution, Deployment → CMake
2. Add Profile using Preset
3. Use built-in build tools

#### Qt Creator

1. File → Open File or Project → Select `CMakeLists.txt`
2. Specify Qt path in Kit configuration
3. Build → Run

---

## Resources

- [CMake Official Documentation](https://cmake.org/documentation/)
- [vcpkg Official Documentation](https://vcpkg.io/)
- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [Project Wiki](https://github.com/cool2528/GDownload/wiki)

---

## Getting Help

If you encounter build issues:

1. Check [GitHub Issues](https://github.com/cool2528/GDownload/issues)
2. Submit a new Issue (attach complete error logs and environment info)
3. Join community discussions

**Information template:**
```
- OS and version:
- Qt version:
- CMake version:
- vcpkg version:
- Compiler version:
- Complete error log:
```
