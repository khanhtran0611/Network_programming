-- Insert test data vào database
-- Chạy: sqlite3 groups.db < insert_test_data.sql

-- Xóa dữ liệu cũ (nếu muốn reset)
DELETE FROM Request;
DELETE FROM Invitation;
DELETE FROM Member;
DELETE FROM Group_table;
DELETE FROM User;

-- Insert Users (15 users)
INSERT INTO User (email, password, username) VALUES
('alice@example.com', 'pass123', 'Alice Johnson'),
('bob@example.com', 'pass123', 'Bob Smith'),
('charlie@example.com', 'pass123', 'Charlie Brown'),
('diana@example.com', 'pass123', 'Diana Prince'),
('eve@example.com', 'pass123', 'Eve Wilson'),
('frank@example.com', 'pass123', 'Frank Miller'),
('grace@example.com', 'pass123', 'Grace Lee'),
('henry@example.com', 'pass123', 'Henry Davis'),
('ivy@example.com', 'pass123', 'Ivy Chen'),
('jack@example.com', 'pass123', 'Jack Taylor'),
('kate@example.com', 'pass123', 'Kate Anderson'),
('liam@example.com', 'pass123', 'Liam O''Brien'),
('mia@example.com', 'pass123', 'Mia Garcia'),
('noah@example.com', 'pass123', 'Noah Martinez'),
('olivia@example.com', 'pass123', 'Olivia White');

-- Insert Groups (10 groups)
INSERT INTO Group_table (name, leader) VALUES
('Family Photos', 'alice@example.com'),
('Work Project X', 'bob@example.com'),
('Travel Plans 2024', 'charlie@example.com'),
('Study Group', 'diana@example.com'),
('Gaming Squad', 'eve@example.com'),
('Book Club', 'frank@example.com'),
('Fitness Team', 'grace@example.com'),
('Music Band', 'henry@example.com'),
('Cooking Class', 'ivy@example.com'),
('Tech Startup', 'jack@example.com');

-- Insert Members (mỗi group có leader + vài members)
-- Group 1: Family Photos
INSERT INTO Member (group_id, Email) VALUES
(1, 'alice@example.com'),
(1, 'bob@example.com'),
(1, 'charlie@example.com'),
(1, 'diana@example.com'),
(1, 'eve@example.com');

-- Group 2: Work Project X
INSERT INTO Member (group_id, Email) VALUES
(2, 'bob@example.com'),
(2, 'frank@example.com'),
(2, 'grace@example.com'),
(2, 'henry@example.com'),
(2, 'ivy@example.com'),
(2, 'jack@example.com'),
(2, 'kate@example.com'),
(2, 'liam@example.com');

-- Group 3: Travel Plans 2024
INSERT INTO Member (group_id, Email) VALUES
(3, 'charlie@example.com'),
(3, 'alice@example.com'),
(3, 'mia@example.com');

-- Group 4: Study Group
INSERT INTO Member (group_id, Email) VALUES
(4, 'diana@example.com'),
(4, 'eve@example.com'),
(4, 'frank@example.com'),
(4, 'grace@example.com'),
(4, 'henry@example.com'),
(4, 'ivy@example.com'),
(4, 'jack@example.com'),
(4, 'kate@example.com'),
(4, 'liam@example.com'),
(4, 'mia@example.com');

-- Group 5: Gaming Squad
INSERT INTO Member (group_id, Email) VALUES
(5, 'eve@example.com'),
(5, 'noah@example.com'),
(5, 'olivia@example.com'),
(5, 'alice@example.com');

-- Group 6: Book Club
INSERT INTO Member (group_id, Email) VALUES
(6, 'frank@example.com'),
(6, 'grace@example.com'),
(6, 'kate@example.com');

-- Group 7: Fitness Team
INSERT INTO Member (group_id, Email) VALUES
(7, 'grace@example.com'),
(7, 'henry@example.com'),
(7, 'liam@example.com'),
(7, 'mia@example.com');

-- Group 8: Music Band
INSERT INTO Member (group_id, Email) VALUES
(8, 'henry@example.com'),
(8, 'ivy@example.com'),
(8, 'jack@example.com');

-- Group 9: Cooking Class
INSERT INTO Member (group_id, Email) VALUES
(9, 'ivy@example.com'),
(9, 'diana@example.com'),
(9, 'olivia@example.com');

-- Group 10: Tech Startup
INSERT INTO Member (group_id, Email) VALUES
(10, 'jack@example.com'),
(10, 'bob@example.com'),
(10, 'charlie@example.com'),
(10, 'frank@example.com');

-- Insert Requests (một số pending, một số approved)
-- Pending requests
INSERT INTO Request (email, group_id, status) VALUES
('noah@example.com', 1, 'pending'),
('olivia@example.com', 2, 'pending'),
('alice@example.com', 6, 'pending'),
('bob@example.com', 7, 'pending'),
('charlie@example.com', 9, 'pending');

-- Approved requests (đã được approve nhưng chưa thêm vào Member - chỉ để test)
INSERT INTO Request (email, group_id, status) VALUES
('diana@example.com', 5, 'approved'),
('eve@example.com', 8, 'approved');

-- Rejected requests
INSERT INTO Request (email, group_id, status) VALUES
('frank@example.com', 3, 'rejected'),
('grace@example.com', 4, 'rejected');

-- Insert Invitations (một số pending, một số accepted)
-- Pending invitations
INSERT INTO Invitation (sender_email, receiver_email, group_id, status) VALUES
('alice@example.com', 'noah@example.com', 1, 'pending'),
('bob@example.com', 'olivia@example.com', 2, 'pending'),
('charlie@example.com', 'alice@example.com', 3, 'pending'),
('diana@example.com', 'bob@example.com', 4, 'pending');

-- Accepted invitations
INSERT INTO Invitation (sender_email, receiver_email, group_id, status) VALUES
('eve@example.com', 'charlie@example.com', 5, 'accepted'),
('frank@example.com', 'diana@example.com', 6, 'accepted');

-- Rejected invitations
INSERT INTO Invitation (sender_email, receiver_email, group_id, status) VALUES
('grace@example.com', 'eve@example.com', 7, 'rejected'),
('henry@example.com', 'frank@example.com', 8, 'rejected');

-- Hiển thị thống kê
SELECT '=== SUMMARY ===' as '';
SELECT 'Total Users: ' || COUNT(*) FROM User;
SELECT 'Total Groups: ' || COUNT(*) FROM Group_table;
SELECT 'Total Members: ' || COUNT(*) FROM Member;
SELECT 'Pending Requests: ' || COUNT(*) FROM Request WHERE status = 'pending';
SELECT 'Total Invitations: ' || COUNT(*) FROM Invitation;

