#!/bin/bash

SERVICE_FILE="/etc/systemd/system/raspberrypi-lvgl-terminal.service"

show_help() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  --enable     启用开机自启动运行demo"
    echo "  --disable    禁用开机自启动并停止当前运行的demo"
    echo "  -h, --help   显示此帮助信息"
}

enable_service() {
    if [ ! -f "./demo" ]; then
        echo "错误：当前目录下未找到 demo 文件"
        echo "请先运行 build.sh 脚本编译项目"
        exit 1
    fi

    echo "创建开机自启动服务..."

    sudo tee "$SERVICE_FILE" > /dev/null << EOF
[Unit]
Description=Raspberry Pi LVGL Terminal Application
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$(pwd)
ExecStart=$(pwd)/demo
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

    sudo systemctl daemon-reload

    sudo systemctl enable raspberrypi-lvgl-terminal.service
    sudo systemctl start raspberrypi-lvgl-terminal.service

    echo "服务已启用并启动"
    echo "状态："
    sudo systemctl status raspberrypi-lvgl-terminal.service --no-pager -l
}

disable_service() {
    echo "禁用开机自启动服务..."

    if sudo systemctl is-active --quiet raspberrypi-lvgl-terminal.service; then
        sudo systemctl stop raspberrypi-lvgl-terminal.service
        echo "服务已停止"
    else
        echo "服务未运行"
    fi

    if sudo systemctl is-enabled --quiet raspberrypi-lvgl-terminal.service; then
        sudo systemctl disable raspberrypi-lvgl-terminal.service
        echo "服务已禁用开机自启动"
    else
        echo "服务未设置为开机自启动"
    fi

    if [ -f "$SERVICE_FILE" ]; then
        sudo rm "$SERVICE_FILE"
        sudo systemctl daemon-reload
        echo "服务文件已删除"
    fi

    echo "服务已禁用"
}

case "$1" in
    --enable)
        enable_service
        ;;
    --disable)
        disable_service
        ;;
    -h|--help)
        show_help
        ;;
    "")
        echo "请选择一个选项："
        show_help
        exit 1
        ;;
    *)
        echo "未知选项: $1"
        show_help
        exit 1
        ;;
esac