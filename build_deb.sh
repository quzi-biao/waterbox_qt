#!/bin/bash
set -e

echo "=== WaterBox Qt DEB 打包脚本 ==="

# 安装编译依赖
echo "[1/4] 检查并安装依赖..."
sudo apt update
sudo apt install -y build-essential cmake \
    qt6-base-dev libqt6charts6-dev \
    libqt6sql6-sqlite \
    libssl-dev zlib1g-dev

# 编译
echo "[2/4] 编译项目..."
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 打包
echo "[3/4] 生成 DEB 包..."
cpack -G DEB

# 输出结果
echo "[4/4] 打包完成"
DEB_FILE=$(ls *.deb 2>/dev/null | head -1)
if [ -n "$DEB_FILE" ]; then
    echo "生成文件: $(pwd)/$DEB_FILE"
    echo "安装命令: sudo dpkg -i $DEB_FILE"
else
    echo "错误: 未找到 .deb 文件"
    exit 1
fi
