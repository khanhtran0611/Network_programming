# Lệnh chạy nhanh

## Copy và paste từng bước vào WSL Ubuntu:

### Bước 1: Mở WSL Ubuntu
```powershell
wsl -d Ubuntu
```

### Bước 2: Di chuyển đến thư mục và cài dependencies
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming
sudo apt-get update && sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

### Bước 3: Compile
```bash
gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3 && gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`
```

### Bước 4: Chạy Server (Terminal 1)
```bash
./server
```

### Bước 5: Chạy Client (Terminal 2 - mở WSL mới)
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming && ./client_group
```

---

## Hoặc dùng script tự động:

### Copy toàn bộ và paste vào WSL:
```bash
cd /mnt/d/Data\ ML/Network_Programming/Network_programming && sudo apt-get update -qq && sudo apt-get install -y build-essential libsqlite3-dev libgtk-3-dev pkg-config && gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3 && gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0` && echo "✓ Compile thành công! Chạy ./server và ./client_group"
```

