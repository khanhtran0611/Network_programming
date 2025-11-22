#!/bin/bash
# Script để tạo user mẫu cho testing

DB_FILE="groups.db"

# Kiểm tra database có tồn tại không
if [ ! -f "$DB_FILE" ]; then
    echo "Database chưa tồn tại. Vui lòng chạy server một lần để tạo database."
    exit 1
fi

# Tạo các user mẫu
sqlite3 "$DB_FILE" <<EOF
-- Xóa dữ liệu cũ (nếu muốn reset)
-- DELETE FROM Request;
-- DELETE FROM Member;
-- DELETE FROM Group_table;
-- DELETE FROM User;

-- Tạo các user mẫu
INSERT OR IGNORE INTO User (email, password, username) VALUES 
('user1@example.com', 'pass1', 'User One'),
('user2@example.com', 'pass2', 'User Two'),
('user3@example.com', 'pass3', 'User Three'),
('leader@example.com', 'leader', 'Group Leader'),
('member@example.com', 'member', 'Group Member');

-- Tạo một nhóm mẫu
INSERT OR IGNORE INTO Group_table (name, leader) VALUES 
('Sample Group', 'leader@example.com');

-- Thêm leader vào nhóm
INSERT OR IGNORE INTO Member (group_id, Email) 
SELECT group_id, leader FROM Group_table WHERE name = 'Sample Group';

-- Thêm một member mẫu
INSERT OR IGNORE INTO Member (group_id, Email) 
SELECT group_id, 'member@example.com' FROM Group_table WHERE name = 'Sample Group';

SELECT 'Users created successfully!' as Status;
SELECT email, username FROM User;
EOF

echo ""
echo "Đã tạo user mẫu thành công!"
echo "Bạn có thể sử dụng các email sau để test:"
echo "  - leader@example.com (password: leader)"
echo "  - member@example.com (password: member)"
echo "  - user1@example.com (password: pass1)"
echo "  - user2@example.com (password: pass2)"
echo "  - user3@example.com (password: pass3)"

