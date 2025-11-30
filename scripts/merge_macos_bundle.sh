#!/bin/bash
# 合并两个 .app bundle 为 Universal Binary

set -e

ARM64_APP="$1"
X64_APP="$2"
UNIVERSAL_APP="$3"

contains_all_archs() {
    local source="$1"
    local target="$2"

    for target_arch in $target; do
        local found=1
        for source_arch in $source; do
            if [ "$source_arch" = "$target_arch" ]; then
                found=0
                break
            fi
        done
        if [ $found -ne 0 ]; then
            return 1
        fi
    done

    return 0
}

resolve_bundle_path() {
    local requested="$1"
    local label="$2"

    if [ -d "$requested" ]; then
        echo "$requested"
        return 0
    fi

    local parent_dir
    parent_dir="$(dirname "$requested")"
    local bundle_name
    bundle_name="$(basename "$requested")"

    if [ -d "$parent_dir" ]; then
        local found_path
        found_path="$(find "$parent_dir" -maxdepth 4 -type d -name "$bundle_name" | head -n 1)"
        if [ -n "$found_path" ]; then
            echo "信息: 自动调整 ${label} bundle 路径 -> ${found_path}"
            echo "$found_path"
            return 0
        fi
    fi

    return 1
}

ARM64_APP_RESOLVED="$(resolve_bundle_path "$ARM64_APP" "ARM64")" || {
    echo "错误: 找不到输入的 ARM64 .app bundle ($ARM64_APP)"
    exit 1
}
X64_APP_RESOLVED="$(resolve_bundle_path "$X64_APP" "x64")" || {
    echo "错误: 找不到输入的 x64 .app bundle ($X64_APP)"
    exit 1
}
ARM64_APP="$ARM64_APP_RESOLVED"
X64_APP="$X64_APP_RESOLVED"

# 验证输入
if [ ! -d "$ARM64_APP" ] || [ ! -d "$X64_APP" ]; then
    echo "错误: 找不到输入的 .app bundle"
    exit 1
fi

# 复制 ARM64 版本作为基础
echo "复制 ARM64 版本作为基础..."
mkdir -p "$(dirname "$UNIVERSAL_APP")"
if [ -e "$UNIVERSAL_APP" ]; then
    rm -rf "$UNIVERSAL_APP"
fi
cp -R "$ARM64_APP" "$UNIVERSAL_APP"

# 查找所有可执行文件和动态库
echo "开始合并二进制文件..."
find "$ARM64_APP" -type f \( -perm -111 -o -name "*.dylib" -o -path "*/Contents/MacOS/*" \) | while read -r ARM64_FILE; do
    RELATIVE_PATH="${ARM64_FILE#$ARM64_APP/}"
    X64_FILE="$X64_APP/$RELATIVE_PATH"
    UNIVERSAL_FILE="$UNIVERSAL_APP/$RELATIVE_PATH"

    if ! file "$ARM64_FILE" | grep -q "Mach-O"; then
        continue
    fi

    if [ -f "$X64_FILE" ]; then
        echo "合并: $RELATIVE_PATH"
        ARM64_ARCHS="$(lipo -archs "$ARM64_FILE" 2>/dev/null || echo "")"
        X64_ARCHS="$(lipo -archs "$X64_FILE" 2>/dev/null || echo "")"
        if [ -n "$ARM64_ARCHS" ] && [ -n "$X64_ARCHS" ] && contains_all_archs "$ARM64_ARCHS" "$X64_ARCHS"; then
            echo "信息: $RELATIVE_PATH ARM64 版本已包含所有所需架构 (${ARM64_ARCHS})，跳过合并"
            cp "$ARM64_FILE" "$UNIVERSAL_FILE"
            continue
        fi
        lipo -create "$ARM64_FILE" "$X64_FILE" -output "$UNIVERSAL_FILE"
    else
        echo "警告: $RELATIVE_PATH 在 x64 版本中不存在，保留 ARM64 版本"
    fi
done

echo "Universal Binary 创建成功: $UNIVERSAL_APP"
