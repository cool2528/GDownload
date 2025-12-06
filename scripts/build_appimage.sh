#!/usr/bin/env bash
# 构建 Linux AppImage 包

set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "用法: $0 <APPDIR> <OUTPUT_APPIMAGE>"
    exit 1
fi

APPDIR="$1"
OUTPUT_PATH="$2"

if [[ ! -d "$APPDIR" ]]; then
    echo "找不到 AppDir: $APPDIR"
    exit 1
fi

LINUXDEPLOY_BIN="$(command -v linuxdeploy || true)"
if [[ -z "$LINUXDEPLOY_BIN" ]]; then
    echo "找不到 linuxdeploy，可执行文件未安装到 PATH"
    exit 1
fi

if [[ -f /usr/lib/x86_64-linux-gnu/libtiff.so.6 && ! -f /usr/lib/x86_64-linux-gnu/libtiff.so.5 ]]; then
    sudo ln -sf /usr/lib/x86_64-linux-gnu/libtiff.so.6 /usr/lib/x86_64-linux-gnu/libtiff.so.5
fi

TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

mkdir -p "$(dirname "$OUTPUT_PATH")"

export APPIMAGE_EXTRACT_AND_RUN=1
export APPIMAGE_NAME="$(basename "$OUTPUT_PATH")"
export LD_LIBRARY_PATH="$APPDIR/lib:$APPDIR/usr/lib:${LD_LIBRARY_PATH:-}"
export UPDATE_INFORMATION="gh-releases-zsync|cool2528|GDownload|latest|gdownload-*.AppImage.zsync"
# QML 依赖收集：让 linuxdeploy-plugin-qt 找到实际的 QML 源文件
if [[ -z "${QML_SOURCES_PATHS:-}" ]]; then
    SCRIPT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    export QML_SOURCES_PATHS="${SCRIPT_ROOT}/src/App/ui/Resource/qml"
fi

if [[ -n "${QTDIR:-}" ]]; then
    export QT_PLUGIN_PATH="$QTDIR/plugins"
    export QML2_IMPORT_PATH="$QTDIR/qml"
    export QMAKE="$QTDIR/bin/qmake"
fi

pushd "$TMPDIR" >/dev/null
"$LINUXDEPLOY_BIN" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/gdownload" \
    --desktop-file "$APPDIR/usr/share/applications/gdownload.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/gdownload.svg" \
    --plugin qt \
    --output appimage

GENERATED_APPIMAGE="$(ls -1 *.AppImage 2>/dev/null | tail -n 1 || true)"
if [[ -z "$GENERATED_APPIMAGE" ]]; then
    echo "linuxdeploy 未生成 AppImage"
    exit 1
fi

mv "$GENERATED_APPIMAGE" "$OUTPUT_PATH"
popd >/dev/null

echo "AppImage 已生成: $OUTPUT_PATH"
