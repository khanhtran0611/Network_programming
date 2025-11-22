#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DB_PATH "groups.db"
#define MAX_EMAIL_LEN 30
#define MAX_USERNAME_LEN 100
#define MAX_GROUP_NAME_LEN 50
#define MAX_STATUS_LEN 30

// Initialize database and create tables
int init_database(void);

// Group operations
int create_group(const char *group_name, const char *leader_email);
int list_groups(char *buffer, size_t buffer_size);
int get_group_id_by_name(const char *group_name);

// Member operations
int add_member(int group_id, const char *email);
int remove_member(int group_id, const char *email);
int list_members(int group_id, char *buffer, size_t buffer_size);
int get_member_count(int group_id);
int is_member(int group_id, const char *email);
int is_leader(int group_id, const char *email);

// Request operations
int create_request(const char *email, int group_id);
int approve_request(int request_id);
int reject_request(int request_id);
int list_requests(int group_id, char *buffer, size_t buffer_size);
int get_request_id(const char *email, int group_id);
int get_request_group_id(int request_id);

// User operations
int user_exists(const char *email);
int create_user(const char *email, const char *password, const char *username);
int verify_login(const char *email, const char *password, char *username, size_t username_size);
int register_user(const char *email, const char *password, const char *username);

#endif  // DATABASE_H

