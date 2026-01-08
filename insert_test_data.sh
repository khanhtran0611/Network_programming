#!/bin/bash
# Script insert test data vào database

echo "Inserting test data into groups.db..."
sqlite3 groups.db < insert_test_data.sql

echo ""
echo "✅ Test data đã được insert!"
echo ""
echo "Xem dữ liệu:"
sqlite3 groups.db "SELECT 'Users:' as ''; SELECT email, username FROM User LIMIT 5; SELECT '...' as ''; SELECT 'Groups:' as ''; SELECT group_id, name, leader FROM Group_table; SELECT 'Members (sample):' as ''; SELECT group_id, Email FROM Member LIMIT 10;"

