#!/bin/bash

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WIRINGPI_DIR="$REPO_ROOT/WiringPi"
WIFI_IFACE="${WIFI_IFACE:-wlan0}"
HOTSPOT_CONN_NAME="${HOTSPOT_CONN_NAME:-PiTerminalHotspot}"
HOTSPOT_ADDR="${HOTSPOT_ADDR:-10.42.0.1/24}"

log() {
    echo "[build] $*"
}

wifi_init_log() {
    echo "[wifi-init] $*"
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        wifi_init_log "缺少命令: $1"
        exit 1
    fi
}

run_wifi_checks() {
    local device_type
    local managed_state
    local ap_supported

    require_command systemctl
    require_command nmcli
    require_command ip

    wifi_init_log "确保 NetworkManager 已启用..."
    sudo systemctl enable NetworkManager >/dev/null 2>&1 || true
    sudo systemctl start NetworkManager >/dev/null 2>&1 || true

    if ! systemctl is-active --quiet NetworkManager; then
        wifi_init_log "NetworkManager 未成功启动"
        exit 1
    fi

    wifi_init_log "打开 Wi-Fi 无线电..."
    sudo nmcli radio wifi on >/dev/null 2>&1 || true

    if ! ip link show "$WIFI_IFACE" >/dev/null 2>&1; then
        wifi_init_log "未找到无线接口 $WIFI_IFACE"
        exit 1
    fi

    device_type="$(nmcli -t -f DEVICE,TYPE device status | awk -F: -v dev="$WIFI_IFACE" '$1 == dev {print $2; exit}')"
    if [ "$device_type" != "wifi" ]; then
        wifi_init_log "$WIFI_IFACE 不是 NetworkManager 识别的 Wi-Fi 接口"
        exit 1
    fi

    wifi_init_log "确保 $WIFI_IFACE 由 NetworkManager 管理..."
    sudo nmcli device set "$WIFI_IFACE" managed yes >/dev/null 2>&1 || true

    managed_state="$(nmcli -t -f GENERAL.NM-MANAGED device show "$WIFI_IFACE" 2>/dev/null | awk -F: 'NR==1 {print $2}')"
    if [ "$managed_state" != "yes" ]; then
        wifi_init_log "$WIFI_IFACE 当前不是由 NetworkManager 管理"
        exit 1
    fi

    ap_supported="$(nmcli -t -f WIFI-PROPERTIES.AP device show "$WIFI_IFACE" 2>/dev/null | awk -F: 'NR==1 {print $2}')"
    if [ "$ap_supported" != "yes" ]; then
        wifi_init_log "$WIFI_IFACE 不支持 AP/热点模式"
        exit 1
    fi

    if nmcli -t -f NAME connection show | grep -Fxq "$HOTSPOT_CONN_NAME"; then
        wifi_init_log "更新已有热点配置 $HOTSPOT_CONN_NAME ..."
        sudo nmcli connection modify "$HOTSPOT_CONN_NAME" \
            ipv4.method shared \
            ipv4.addresses "$HOTSPOT_ADDR" \
            ipv6.method ignore \
            connection.autoconnect no >/dev/null
    else
        wifi_init_log "尚未创建热点配置 $HOTSPOT_CONN_NAME，程序首次打开热点时会自动创建"
        wifi_init_log "期望热点地址固定为 ${HOTSPOT_ADDR%/*}"
    fi

    wifi_init_log "检查完成：NetworkManager、$WIFI_IFACE、热点模式均满足运行条件"
}

echo "开始构建 raspberrypi-lvgl-terminal..."

cd "$REPO_ROOT"

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
    wireless-tools \
    wpasupplicant \
    python3 \
    python3-opencv \
    python3-numpy \
    git

if [ "${SKIP_WIFI_INIT:-0}" != "1" ]; then
    echo "执行 Wi-Fi 初始化检查..."
    run_wifi_checks
else
    echo "已跳过 Wi-Fi 初始化检查（SKIP_WIFI_INIT=1）"
fi

if [ ! -d "$WIRINGPI_DIR/.git" ]; then
    log "拉取 WiringPi..."
    git clone https://github.com/WiringPi/WiringPi.git "$WIRINGPI_DIR"
else
    log "检测到已有 WiringPi 目录，跳过重新拉取..."
fi

log "构建 WiringPi..."
cd "$WIRINGPI_DIR"
./build
cd "$REPO_ROOT"

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
