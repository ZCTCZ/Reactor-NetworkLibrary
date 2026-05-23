#!/bin/bash
set -x
# 执行./auto-building.sh，则默认使用 --preset=clang-debug 构建
# 可以指定构建方式，如：./auto-building.sh gcc-debug

PRESET="${1:-clang-debug}"
BUILD_DIR="$PWD/build/$PRESET"

# 清理旧文件（如果有）
rm -rf "$BUILD_DIR"

# 使用 preset 配置并编译
cmake --preset="$PRESET"
cmake --build --preset="$PRESET"