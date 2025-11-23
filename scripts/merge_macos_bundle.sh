#!/bin/bash
# 合并两个 .app bundle 为 Universal Binary

set -e

ARM64_APP="$1"
X64_APP="$2"
UNIVERSAL_APP="$3"

# 验证输入
if [ ! -d "$ARM64_APP" ] || [ ! -d "$X64_APP" ]; then
    echo "错误: 找不到输入的 .app bundle"
    exit 1
fi

# 复制 ARM64 版本作为基础
echo "复制 ARM64 版本作为基础..."
cp -R "$ARM64_APP" "$UNIVERSAL_APP"

# 查找所有可执行文件和动态库
echo "开始合并二进制文件..."
find "$ARM64_APP" \( -type f -perm +111 -o -name "*.dylib" \) | while read -r ARM64_FILE; do
    RELATIVE_PATH="${ARM64_FILE#$ARM64_APP/}"
    X64_FILE="$X64_APP/$RELATIVE_PATH"
    UNIVERSAL_FILE="$UNIVERSAL_APP/$RELATIVE_PATH"

    if [ -f "$X64_FILE" ]; then
        echo "合并: $RELATIVE_PATH"
        lipo -create "$ARM64_FILE" "$X64_FILE" -output "$UNIVERSAL_FILE"
    else
        echo "警告: $RELATIVE_PATH 在 x64 版本中不存在，保留 ARM64 版本"
    fi
done

echo "Universal Binary 创建成功: $UNIVERSAL_APP"
