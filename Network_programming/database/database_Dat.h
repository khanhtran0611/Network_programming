#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

/**
 * @brief Khởi tạo kết nối CSDL
 * @param db_path Đường dẫn tới file app.db
 * @return Con trỏ sqlite3*, NULL nếu lỗi.
 */
sqlite3* db_connect(const char* db_path);

/**
 * @brief Đăng ký người dùng mới
 * @param db Con trỏ CSDL
 * @param email Email (khóa chính)
 * @param username Tên hiển thị
 * @param password Mật khẩu (plaintext)
 * @return 0 nếu thành công, 1 nếu email đã tồn tại, -1 nếu lỗi khác.
 */
int db_register_user(sqlite3 *db, const char *email, const char *username, const char *password);

// ... (Sau này thêm: db_login_user, db_create_group, v.v...)

/**
 * @brief Xác thực đăng nhập người dùng
 * @param db Con trỏ CSDL
 * @param email Email người dùng nhập
 * @param password Mật khẩu (plaintext) người dùng nhập
 * @return 0 nếu đăng nhập thành công,
 * 1 nếu không tìm thấy email,
 * 2 nếu sai mật khẩu,
 * -1 nếu lỗi SQL hoặc lỗi khác.
 */
int db_login_user(sqlite3 *db, const char *email, const char *password);

// Tạo session mới, trả về 0 nếu thành công
int db_create_session(sqlite3 *db, const char *token, const char *user_email);

// Lấy email từ token. Trả về email (cần free) hoặc NULL nếu không tìm thấy.
char* db_get_email_by_token(sqlite3 *db, const char *token);

// Xóa session (Đăng xuất)
void db_remove_session(sqlite3 *db, const char *token);
#endif