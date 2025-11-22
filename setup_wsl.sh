#!/bin/bash
# Script tự động setup và compile trong WSL

echo "=== Setup Group Management System trong WSL ==="
echo ""

# Kiểm tra đang ở đúng thư mục
if [ ! -f "server.c" ] || [ ! -f "database.c" ]; then
    echo "Lỗi: Không tìm thấy file source code!"
    echo "Vui lòng chạy script này trong thư mục Network_programming"
    exit 1
fi

# Cài đặt dependencies
echo "Bước 1: Kiểm tra và cài đặt dependencies..."
if ! command -v gcc &> /dev/null; then
    echo "Cài đặt build-essential..."
    sudo apt-get update
    sudo apt-get install -y build-essential
fi

if ! pkg-config --exists sqlite3; then
    echo "Cài đặt SQLite3..."
    sudo apt-get install -y libsqlite3-dev
fi

if ! pkg-config --exists gtk+-3.0; then
    echo "Cài đặt GTK+3..."
    sudo apt-get install -y libgtk-3-dev pkg-config
fi

echo "✓ Dependencies đã sẵn sàng"
echo ""

# Compile
echo "Bước 2: Compile code..."
if [ -f "Makefile" ]; then
    make clean
    make all
else
    echo "Compile server..."
    gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3
    if [ $? -ne 0 ]; then
        echo "Lỗi compile server!"
        exit 1
    fi
    
    echo "Compile client_group..."
    gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`
    if [ $? -ne 0 ]; then
        echo "Lỗi compile client_group!"
        exit 1
    fi
fi

echo "✓ Compile thành công!"
echo ""

# Kiểm tra files
if [ -f "server" ] && [ -f "client_group" ]; then
    echo "=== Setup hoàn tất! ==="
    echo ""
    echo "Để chạy:"
    echo "  Terminal 1: ./server"
    echo "  Terminal 2: ./client_group"
    echo ""
    echo "Lưu ý: Nếu dùng Windows 10, cần cài VcXsrv để hiển thị GUI"
else
    echo "Lỗi: Compile không thành công!"
    exit 1
fi

