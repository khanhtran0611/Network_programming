# Hướng dẫn nhanh - Chạy trong WSL

Bạn đã có WSL Ubuntu! Làm theo các bước sau:

## Bước 1: Khởi động WSL

Trong Git Bash hoặc PowerShell:
```bash
wsl
```

Hoặc mở **Ubuntu** từ Start Menu.

## Bước 2: Cài đặt dependencies (chỉ cần làm 1 lần)

```bash
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

## Bước 3: Di chuyển đến thư mục project

```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
```

## Bước 4: Compile

```bash
make clean
make all
```

Hoặc compile thủ công:
```bash
gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3
gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`
```

## Bước 5: Chạy

**Terminal 1 (WSL):**
```bash
./server
```

**Terminal 2 (Mở WSL terminal mới):**
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
./client_group
```

## Lưu ý về GUI

### Windows 11:
- WSLg tự động hỗ trợ GUI, không cần cài thêm

### Windows 10:
- Cần cài VcXsrv X Server
- Download: https://sourceforge.net/projects/vcxsrv/
- Chạy XLaunch → chọn "Multiple windows"
- Trong WSL:
```bash
export DISPLAY=:0
./client_group
```

## Test nhanh

1. Tạo nhóm: Click nút "+" → Nhập tên → Create
2. Xem nhóm: Double-click vào nhóm
3. Xem thành viên: Tab "Members"
4. Xin vào nhóm: Right-click nhóm → "Join Group"
5. Phê duyệt: Tab "Approval Requests" → "Approve"
6. Rời nhóm: Button "Leave Group"

