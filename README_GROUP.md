# Hướng dẫn nhanh - Group Management

## Compile và chạy

### 1. Compile
```bash
make clean
make all
```

### 2. Chạy Server (Terminal 1)
```bash
./server
```

### 3. Chạy Client (Terminal 2)
```bash
./client_group
```

## Test nhanh

1. **Tạo nhóm:** Click nút "+" → Nhập tên → Create
2. **Xem nhóm:** Double-click vào nhóm trong danh sách
3. **Xem thành viên:** Tab "Members"
4. **Xin vào nhóm:** Right-click nhóm → "Join Group"
5. **Phê duyệt:** Tab "Approval Requests" → Click "Approve"
6. **Rời nhóm:** Button "Leave Group" ở dưới cùng

## Tạo user mẫu (Tùy chọn)

```bash
# Linux/WSL
./create_test_users.sh

# Hoặc thủ công
sqlite3 groups.db "INSERT INTO User (email, password, username) VALUES ('test@example.com', 'test', 'Test User');"
```

## Kiểm tra database

```bash
sqlite3 groups.db
.tables
SELECT * FROM Group_table;
SELECT * FROM Member;
SELECT * FROM Request;
```

Xem chi tiết trong file `TEST_GUIDE.md`

