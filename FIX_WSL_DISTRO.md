# Sửa lỗi WSL Distribution

## Vấn đề
Bạn đang dùng Alpine Linux (hoặc distribution khác) thay vì Ubuntu. Alpine không có `sudo` và cần cài đặt khác.

## Giải pháp 1: Chuyển sang Ubuntu (Khuyến nghị)

### Bước 1: Kiểm tra distributions đã cài
```powershell
wsl --list --verbose
```

### Bước 2: Cài Ubuntu nếu chưa có
```powershell
wsl --install -d Ubuntu
```

### Bước 3: Khởi động Ubuntu
```powershell
wsl -d Ubuntu
```

### Bước 4: Trong Ubuntu, cài dependencies
```bash
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

## Giải pháp 2: Dùng Alpine Linux (Nếu muốn tiếp tục)

### Trong Alpine:
```bash
# Cài đặt sudo
apk add sudo

# Hoặc chạy trực tiếp với root (không cần sudo)
apk update
apk add build-base sqlite-dev gtk+3.0-dev pkgconfig
```

**Lưu ý:** Alpine dùng `apk` thay vì `apt-get`, và package names khác.

## Giải pháp 3: Set Ubuntu làm default

```powershell
wsl --set-default Ubuntu
```

Sau đó chỉ cần chạy `wsl` sẽ vào Ubuntu.

