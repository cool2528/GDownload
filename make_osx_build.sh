#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Print each command before executing it
set -x

# Define variables
QT_DIR="/Applications/qt/6.5.2/macos"
BUILD_DIR="build"

# Ensure the script is run from the project root directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: This script must be run from the project root directory containing CMakeLists.txt"
    exit 1
fi

# If BUILD_DIR exists, delete it
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

cmake -S . -B build -DQt6_DIR="$QT_DIR/lib/cmake/Qt6" -DCMAKE_PREFIX_PATH=$QT_DIR -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
