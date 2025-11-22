# Chuyển sang Ubuntu WSL

## Vấn đề
Bạn đang dùng docker-desktop (Alpine Linux) thay vì Ubuntu.

## Giải pháp: Chuyển sang Ubuntu

### Bước 1: Set Ubuntu làm default
Trong PowerShell:
```powershell
wsl --set-default Ubuntu
```

### Bước 2: Khởi động Ubuntu
```powershell
wsl
```

Hoặc chỉ định rõ:
```powershell
wsl -d Ubuntu
```

### Bước 3: Trong Ubuntu, cài dependencies
```bash
sudo apt-get update
sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

### Bước 4: Di chuyển đến thư mục project
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
```

### Bước 5: Compile và chạy
```bash
make clean
make all
./server
```

## Lưu ý
- Mỗi lần chạy `wsl` sẽ vào Ubuntu (vì đã set default)
- Nếu muốn vào docker-desktop: `wsl -d docker-desktop`
- Ubuntu đã được cài sẵn, chỉ cần chuyển sang dùng nó

