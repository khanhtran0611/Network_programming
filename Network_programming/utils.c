#include "shared/utils.h"  // Đường dẫn tương đối

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *hash_password(const char *password)
{
    char *hashed_password = (char *)malloc(crypto_pwhash_STRBYTES);
    if (hashed_password == NULL)
    {
        perror("Failed to allocate memory for hash");
        return NULL;
    }

    if (crypto_pwhash_str(hashed_password, password, strlen(password),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        fprintf(stderr, "Failed to hash password\n");
        free(hashed_password);
        return NULL;
    }

    return hashed_password;
}

int verify_password(const char *hash, const char *password)
{
    if (crypto_pwhash_str_verify(hash, password, strlen(password)) != 0)
    {
        // Mật khẩu không khớp
        return -1;
    }
    // Khớp!
    return 0;
}