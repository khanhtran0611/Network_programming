# Hướng dẫn Test Group Management System

## Yêu cầu hệ thống

- Linux/Unix hoặc WSL (Windows Subsystem for Linux)
- GCC compiler
- SQLite3 development libraries
- GTK+3 development libraries

### Cài đặt dependencies (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install build-essential libsqlite3-dev libgtk-3-dev pkg-config
```

## Bước 1: Compile

### Cách 1: Sử dụng Makefile (Khuyến nghị)
```bash
cd Network_programming
make clean
make all
```

### Cách 2: Compile thủ công

**Compile Server:**
```bash
gcc -Wall -g -std=c11 -o server server.c database.c -lsqlite3
```

**Compile Client Group:**
```bash
gcc -Wall -g -std=c11 `pkg-config --cflags gtk+-3.0` -o client_group client_group.c `pkg-config --libs gtk+-3.0`
```

## Bước 2: Tạo User mẫu (Tùy chọn)

Trước khi test, bạn có thể tạo một số user mẫu trong database:

```bash
sqlite3 groups.db <<EOF
INSERT INTO User (email, password, username) VALUES 
('user1@example.com', 'pass1', 'User One'),
('user2@example.com', 'pass2', 'User Two'),
('user3@example.com', 'pass3', 'User Three');
EOF
```

Hoặc chạy script tạo user:
```bash
sqlite3 groups.db "INSERT INTO User (email, password, username) VALUES ('test@example.com', 'test', 'Test User');"
```

## Bước 3: Chạy Server

Mở terminal 1:
```bash
cd Network_programming
./server
```

Server sẽ:
- Tạo database `groups.db` nếu chưa có
- Tạo các bảng cần thiết
- Lắng nghe trên port 8080
- Hiển thị log các command nhận được

## Bước 4: Chạy Client

Mở terminal 2 (hoặc terminal mới):
```bash
cd Network_programming
./client_group
```

**Lưu ý:** Đảm bảo file `download_folder/test.glade` và `group_members_screen.glade` tồn tại.

## Bước 5: Test các chức năng

### Test 1: Tạo nhóm
1. Click vào nút "+" (Create Group) ở góc trên bên phải
2. Nhập tên nhóm (ví dụ: "Test Group")
3. Click "Create"
4. **Kết quả mong đợi:** Nhóm xuất hiện trong danh sách với "1 members"

### Test 2: List nhóm
1. Khi mở client, danh sách nhóm sẽ tự động load
2. **Kết quả mong đợi:** Hiển thị tất cả nhóm với tên và số thành viên

### Test 3: Xem chi tiết nhóm và liệt kê thành viên
1. Double-click vào một nhóm trong danh sách
2. Cửa sổ mới mở ra với 3 tabs: Files, Members, Approval Requests
3. Click vào tab "Members"
4. **Kết quả mong đợi:** Hiển thị danh sách email của các thành viên

### Test 4: Xin vào nhóm (Request Join)
**Cách 1: Từ main window**
1. Right-click vào một nhóm trong danh sách
2. Chọn "Join Group"
3. **Kết quả mong đợi:** Hiển thị thông báo "Request sent successfully"

**Cách 2: Từ group details window**
1. Mở group details (double-click nhóm)
2. Click button "Join Group" (nếu có)

### Test 5: Phê duyệt yêu cầu (Approve Request)
1. Mở group details của nhóm mà bạn là leader
2. Click vào tab "Approval Requests"
3. **Kết quả mong đợi:** Hiển thị danh sách các yêu cầu đang chờ
4. Click button "Approve" bên cạnh email của user muốn phê duyệt
5. **Kết quả mong đợi:** 
   - User được thêm vào nhóm
   - Request biến mất khỏi danh sách
   - Số thành viên trong tab "Members" tăng lên

### Test 6: Rời nhóm (Leave Group)
1. Mở group details của nhóm bạn là thành viên (không phải leader)
2. Click button "Leave Group" ở dưới cùng
3. Xác nhận trong dialog
4. **Kết quả mong đợi:** 
   - Cửa sổ group details đóng lại
   - Bạn không còn là thành viên của nhóm đó
   - Nhóm vẫn tồn tại (nếu bạn không phải leader)

## Test với nhiều user

Để test đầy đủ, bạn cần chạy nhiều client với các user khác nhau:

1. **Terminal 1:** Chạy server
2. **Terminal 2:** Chạy client_group (user1@example.com)
3. **Terminal 3:** Chạy client_group (user2@example.com)

**Lưu ý:** Hiện tại client_group sử dụng email mặc định `user@example.com`. Để test với nhiều user, bạn cần:
- Sửa `current_user_email` trong `client_group.c` 
- Hoặc thêm tham số command line để truyền email

## Kiểm tra Database

Bạn có thể kiểm tra database trực tiếp:

```bash
sqlite3 groups.db

# Xem tất cả nhóm
SELECT * FROM Group_table;

# Xem tất cả thành viên
SELECT * FROM Member;

# Xem tất cả yêu cầu
SELECT * FROM Request;

# Xem thống kê
SELECT g.name, g.leader, COUNT(m.Email) as member_count 
FROM Group_table g 
LEFT JOIN Member m ON g.group_id = m.group_id 
GROUP BY g.group_id;
```

## Troubleshooting

### Lỗi: "Cannot open database"
- Kiểm tra quyền ghi trong thư mục hiện tại
- Đảm bảo SQLite3 đã được cài đặt

### Lỗi: "Error loading UI file"
- Kiểm tra file `download_folder/test.glade` và `group_members_screen.glade` tồn tại
- Đảm bảo đang chạy từ thư mục `Network_programming`

### Lỗi: "connect: Connection refused"
- Đảm bảo server đang chạy
- Kiểm tra IP address trong `client_group.c` (mặc định: 127.0.0.1)
- Kiểm tra port 8080 không bị block

### Lỗi compile GTK+
- Cài đặt: `sudo apt-get install libgtk-3-dev pkg-config`
- Kiểm tra: `pkg-config --modversion gtk+-3.0`

### Lỗi compile SQLite
- Cài đặt: `sudo apt-get install libsqlite3-dev`
- Kiểm tra: `sqlite3 --version`

## Test Script (Tùy chọn)

Bạn có thể tạo script test tự động:

```bash
#!/bin/bash
# test_automated.sh

# Start server in background
./server &
SERVER_PID=$!
sleep 2

# Test commands (sử dụng netcat hoặc telnet)
echo "CREATE_GROUP" | nc localhost 8080
echo "LIST_GROUP" | nc localhost 8080

# Cleanup
kill $SERVER_PID
```

## Kết quả mong đợi

Sau khi test đầy đủ, bạn sẽ có:
- ✅ Database `groups.db` với các bảng đã tạo
- ✅ Thư mục `Group_folders/` với các thư mục nhóm
- ✅ Server log hiển thị các command đã nhận
- ✅ Client UI hoạt động với tất cả chức năng

