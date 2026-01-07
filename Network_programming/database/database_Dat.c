#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../shared/utils.h"  // Để dùng hash_password
#include "database.h"

sqlite3 *db_connect(const char *db_path)
{
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    else
    {
        fprintf(stdout, "Opened database successfully\n");
    }
    return db;
}

int db_register_user(sqlite3 *db, const char *email, const char *username, const char *password)
{
    // 1. Hash mật khẩu
    char *hashed_password = hash_password(password);
    if (hashed_password == NULL)
    {
        return -1;  // Lỗi hash
    }

    // 2. Chuẩn bị câu lệnh SQL (Prepared Statement để chống SQL Injection)
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO User (email, username, password) VALUES (?, ?, ?);";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "DB Error (register): Failed to prepare statement: %s\n",
                sqlite3_errmsg(db));
        free(hashed_password);
        return -1;
    }

    // 3. Bind các giá trị vào câu lệnh
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hashed_password, -1, SQLITE_STATIC);  // Lưu hash

    // 4. Thực thi câu lệnh
    rc = sqlite3_step(stmt);

    // 5. Giải phóng bộ nhớ (quan trọng!)
    free(hashed_password);   // Hash đã được copy vào CSDL, ta có thể free nó
    sqlite3_finalize(stmt);  // Hoàn tất statement

    if (rc == SQLITE_DONE)
    {
        printf("User '%s' registered successfully.\n", username);
        return 0;  // Thành công
    }
    else if (rc == SQLITE_CONSTRAINT)
    {
        fprintf(stderr, "Error: Email '%s' already exists.\n", email);
        return 1;  // Đã tồn tại (lỗi ràng buộc)
    }
    else
    {
        fprintf(stderr, "DB Error (register): SQL step failed: %s (rc: %d)\n", sqlite3_errmsg(db),
                rc);
        return -1;  // Lỗi khác
    }
}

int db_login_user(sqlite3 *db, const char *email, const char *password)
{
    sqlite3_stmt *stmt;
    // Lấy 'password' (là hash) và 'username' từ bảng User
    const char *sql = "SELECT password, username FROM User WHERE email = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement (LOGIN): %s\n", sqlite3_errmsg(db));
        return -1;  // Lỗi SQL
    }

    // Bind email vào câu lệnh
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);

    // Thực thi
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        // Tìm thấy email!
        // Lấy cột 0 (password hash) và cột 1 (username)
        const unsigned char *db_hash = sqlite3_column_text(stmt, 0);
        const unsigned char *db_username = sqlite3_column_text(stmt, 1);

        // Xác thực mật khẩu
        if (verify_password((const char *)db_hash, password) == 0)
        {
            // Mật khẩu khớp!
            printf("Login successful for user: %s\n", db_username);
            sqlite3_finalize(stmt);
            return 0;  // Đăng nhập thành công
        }
        else
        {
            // Mật khẩu sai
            fprintf(stderr, "Login failed: Incorrect password for %s\n", email);
            sqlite3_finalize(stmt);
            return 2;  // Sai mật khẩu
        }
    }
    else if (rc == SQLITE_DONE)
    {
        // Không tìm thấy dòng nào (rc == SQLITE_DONE vì không có kết quả)
        fprintf(stderr, "Login failed: Email '%s' not found.\n", email);
        sqlite3_finalize(stmt);
        return 1;  // Không tìm thấy email
    }
    else
    {
        // Lỗi khi thực thi
        fprintf(stderr, "SQL error (LOGIN): %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;  // Lỗi SQL khác
    }
}

int db_create_session(sqlite3 *db, const char *token, const char *user_email)
{
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO Session (token, user_email) VALUES (?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "DB Error (create_session): Failed to prepare statement: %s\n",
                sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_email, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    // Check lỗi Step (Thực thi)
    if (rc != SQLITE_DONE)
    {
        // --- IN LỖI RA TERMINAL ---
        fprintf(stderr, "Session Error (Step): %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

char *db_get_email_by_token(sqlite3 *db, const char *token)
{
    sqlite3_stmt *stmt;
    // Kiểm tra xem token có tồn tại không
    const char *sql = "SELECT user_email FROM Session WHERE token = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);

    char *result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Tìm thấy! Copy email ra vùng nhớ mới
        const unsigned char *email = sqlite3_column_text(stmt, 0);
        result = strdup((const char *)email);
    }

    sqlite3_finalize(stmt);
    return result;  // Trả về NULL nếu không tìm thấy
}

void db_remove_session(sqlite3 *db, const char *token)
{
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM Session WHERE token = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}