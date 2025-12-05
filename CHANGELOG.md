# Changelog

## v1.0.7

### 🚀 Highlights / 亮点
- Fix Linux AppImage packaging by exporting QML source paths and passing `--qmldir` to linuxdeploy-plugin-qt, ensuring QtQuick.Controls / Qt.labs.platform are included in the bundle.
- Normalize GitHub release notes (convert `<br>` and CRLF to `\n`) before showing in the updater dialog so Markdown renders correctly across platforms.

### 🐛 Bug Fixes / Bug 修复
- 解决 Linux 自动更新因缺失 QML 模块导致无法启动的问题。
- 修复 Windows/Linux 更新弹窗展示的发行说明换行/格式错乱问题。

### 🛠 Build & CI / 构建与 CI
- AppImage 构建脚本默认设置 `QML_SOURCES_PATHS` 指向 `src/App/ui/Resource/qml`，统一插件搜索路径。
- GitHub Actions Linux 流水线沿用新的打包参数，自动产出完整的 AppImage。

---

**Full Changelog**: https://github.com/cool2528/GDownload/compare/v1.0.6...v1.0.7<br>
**完整变更日志**: https://github.com/cool2528/GDownload/compare/v1.0.6...v1.0.7
