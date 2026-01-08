# Sửa lỗi apt-get lock

## Lỗi: "Could not get lock /var/lib/apt/lists/lock"

Có process apt-get khác đang chạy. Giải quyết:

### Cách 1: Đợi process hoàn thành (30 giây)
```bash
sleep 30 && sudo apt-get update
```

### Cách 2: Kill process đang chạy
```bash
sudo killall apt-get
sudo rm /var/lib/apt/lists/lock
sudo rm /var/cache/apt/archives/lock
sudo rm /var/lib/dpkg/lock*
sudo dpkg --configure -a
```

### Cách 3: Kiểm tra và kill process cụ thể
```bash
ps aux | grep apt
sudo kill -9 760  # (thay 760 bằng PID thực tế)
```

Sau đó chạy lại lệnh cài đặt.

