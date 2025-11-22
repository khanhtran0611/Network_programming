# Hướng dẫn Setup trên Windows

## Vấn đề
Code này được viết cho Linux/Unix, cần môi trường Linux để chạy.

## Giải pháp: Sử dụng WSL (Windows Subsystem for Linux)

### Bước 1: Cài đặt WSL

**Trong PowerShell (Run as Administrator):**
```powershell
wsl --install
```

Hoặc nếu đã có WSL:
```powershell
wsl --install -d Ubuntu
```

Sau khi cài xong, **restart máy**.

### Bước 2: Khởi động WSL

Mở **Ubuntu** từ Start Menu, hoặc chạy:
```powershell
wsl
```

### Bước 3: Cài đặt dependencies trong WSL

```bash
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

### Bước 4: Truy cập thư mục project

```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
```

Hoặc nếu đường dẫn khác:
```bash
cd /mnt/[drive-letter]/[path]
```

### Bước 5: Compile và chạy

```bash
# Compile
make clean
make all

# Hoặc compile thủ công
gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3
gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`

# Chạy server (Terminal 1)
./server

# Chạy client (Terminal 2 - mở WSL terminal mới)
./client_group
```

## Cách 2: Dùng MSYS2/MinGW (Phức tạp hơn)

Nếu muốn compile trực tiếp trên Windows:

### Bước 1: Cài đặt MSYS2
Download từ: https://www.msys2.org/

### Bước 2: Cài đặt packages trong MSYS2
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-sqlite
pacman -S mingw-w64-x86_64-gtk3
pacman -S pkg-config
```

### Bước 3: Compile với MinGW
```bash
gcc -Wall -g -std=c11 -o server.exe server.c database.c -lsqlite3
gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group.exe client_group.c `pkg-config --libs gtk+-3.0`
```

**Lưu ý:** Cần chỉnh sửa code để tương thích với Windows (winsock2.h thay vì arpa/inet.h).

## Khuyến nghị

**Dùng WSL** vì:
- ✅ Đơn giản, không cần sửa code
- ✅ Tất cả dependencies có sẵn
- ✅ Code đã được test trên Linux
- ✅ Không cần port code

## Kiểm tra WSL đã cài chưa

```powershell
wsl --list --verbose
```

Nếu chưa có, cài đặt:
```powershell
wsl --install
```

## Troubleshooting WSL

**Lỗi: "WSL 2 requires an update to its kernel component"**
- Download WSL2 kernel update từ Microsoft
- Link: https://aka.ms/wsl2kernel

**Lỗi: "The requested operation could not be completed"**
- Enable Virtual Machine Platform:
```powershell
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
```
- Restart máy

**Lỗi: Không thấy GUI (GTK)**
- Cài đặt X server cho Windows:
  - VcXsrv: https://sourceforge.net/projects/vcxsrv/
  - Hoặc dùng WSLg (Windows 11)

## Chạy GUI trong WSL

### Windows 10:
1. Cài VcXsrv X Server
2. Chạy XLaunch, chọn "Multiple windows"
3. Trong WSL:
```bash
export DISPLAY=:0
./client_group
```

### Windows 11:
WSLg tự động hỗ trợ GUI, không cần cài thêm.

