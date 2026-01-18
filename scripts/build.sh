#!/bin/bash

set -e  

echo "开始构建 raspberrypi-lvgl-terminal..."

echo "更新包管理器..."
sudo apt-get update

echo "安装必要依赖..."
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libopencv-dev \
    libiw-dev \
    network-manager \
    git
git clone https://github.com/WiringPi/WiringPi.git 
cd WiringPi && ./build && cd ..

if [ -f "demo" ]; then
    echo "删除旧的 demo 文件..."
    rm -f demo
fi

if [ -d "build" ]; then
    echo "清理构建目录..."
    rm -rf build
fi

echo "开始编译项目..."
make clean
make

if [ -f "./demo" ]; then
    echo "编译成功！生成的文件：$(ls -lh ./demo)"
    chmod +x ./demo
else
    echo "错误：编译失败，demo 文件未生成"
    exit 1
fi

echo "构建完成！"
