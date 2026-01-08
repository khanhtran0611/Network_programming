#!/bin/bash
# Script chạy tự động - Copy và paste vào WSL Ubuntu

echo "=== Setup và chạy Group Management System ==="
echo ""

# Di chuyển đến thư mục project
cd /mnt/d/Data\ ML/Network_Programming/Network_programming 2>/dev/null || cd "$(dirname "$0")"

# Kiểm tra dependencies
echo "Kiểm tra dependencies..."
if ! command -v gcc &> /dev/null; then
    echo "Cài đặt build-essential..."
    sudo apt-get update -qq
    sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
fi

# Compile
echo "Compile code..."
if [ -f "Makefile" ]; then
    make clean > /dev/null 2>&1
    make all
else
    gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3
    gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`
fi

if [ $? -eq 0 ]; then
    echo "✓ Compile thành công!"
    echo ""
    echo "Để chạy:"
    echo "  Terminal 1: ./server"
    echo "  Terminal 2: ./client_group"
else
    echo "✗ Lỗi compile!"
    exit 1
fi

