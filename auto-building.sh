#!/bin/bash
set -x #调试模式。执行每条命令之前，先把这条命令本身打印到终端

# 执行 ./auto-building.sh，则默认使用 --preset=clang-debug 构建
# 可以指定构建方式和安装选项，如：
#   ./auto-building.sh                             默认 clang-debug，仅编译
#   ./auto-building.sh gcc-debug                   指定 preset，仅编译
#   ./auto-building.sh gcc-release install         编译并安装到 /usr/local
#   ./auto-building.sh gcc-release install ~/mylib 编译并安装到自定义目录

PRESET="${1:-clang-debug}"
DO_INSTALL=false
[ "$2" == "install" ] && DO_INSTALL=true
INSTALL_PREFIX="${3:-/usr/local}" #安装路径

BUILD_DIR="$PWD/build/$PRESET"

# 清理旧文件（如果有）
rm -rf "$BUILD_DIR"

# 使用 preset 配置并编译
cmake --preset="$PRESET"
cmake --build --preset="$PRESET"

# 安装（默认 /usr/local，可指定自定义路径）
if $DO_INSTALL; then
    if [ "$INSTALL_PREFIX" == "/usr/local" ]; then
        echo "=== 安装到 /usr/local ==="
        sudo cmake --install "$BUILD_DIR"
    else
        echo "=== 安装到 $INSTALL_PREFIX ==="
        cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
    fi
fi