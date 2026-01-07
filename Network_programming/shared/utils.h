#ifndef UTILS_H
#define UTILS_H

#include <sodium.h> // Cần cài libsodium-dev

/**
 * @brief Hash mật khẩu bằng Argon2id (chuẩn của libsodium)
 * @param password Mật khẩu gốc
 * @return Một chuỗi hash (cần được free() sau khi dùng), NULL nếu lỗi.
 */
char* hash_password(const char *password);

/**
 * @brief Xác thực mật khẩu với hash
 * @param hash Chuỗi hash lấy từ CSDL
 * @param password Mật khẩu người dùng nhập
 * @return 0 nếu khớp, -1 nếu không khớp hoặc lỗi.
 */
int verify_password(const char *hash, const char *password);

#endif