# Hướng dẫn chạy trên Windows

## Vấn đề
Code này được viết cho Linux/Unix. Trên Windows có 2 cách:

## Cách 1: Sử dụng WSL (Khuyến nghị - Dễ nhất)

### Bước 1: Cài đặt WSL
```powershell
# Chạy trong PowerShell (Admin)
wsl --install
```

### Bước 2: Khởi động WSL
```bash
wsl
```

### Bước 3: Cài đặt dependencies trong WSL
```bash
sudo apt-get update
sudo apt-get install build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

### Bước 4: Compile và chạy trong WSL
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
make clean
make all
./server
```

## Cách 2: Port sang Windows (Phức tạp hơn)

Cần thay đổi:
- `arpa/inet.h` → `winsock2.h` và `ws2tcpip.h`
- Socket API khác nhau
- GTK+3 cần cài đặt riêng cho Windows
- SQLite cần download và link thủ công

**Khuyến nghị:** Dùng WSL vì đơn giản và code đã được test trên Linux.

## Cách 3: Dùng Docker (Nếu có Docker Desktop)

Tạo Dockerfile và chạy trong container Linux.

