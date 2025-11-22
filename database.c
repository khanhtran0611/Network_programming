#define _DEFAULT_SOURCE
#include "database.h"
#include <sys/stat.h>

static sqlite3 *db = NULL;

int init_database(void)
{
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    // Create User table
    char *sql_user = "CREATE TABLE IF NOT EXISTS User ("
                     "email VARCHAR(30) PRIMARY KEY,"
                     "password VARCHAR(30) NOT NULL,"
                     "username VARCHAR(100) NOT NULL);";

    // Create Group table
    char *sql_group = "CREATE TABLE IF NOT EXISTS Group_table ("
                      "group_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "name VARCHAR(50) NOT NULL,"
                      "leader VARCHAR(30) NOT NULL,"
                      "FOREIGN KEY(leader) REFERENCES User(email));";

    // Create Member table
    char *sql_member = "CREATE TABLE IF NOT EXISTS Member ("
                       "group_id INTEGER NOT NULL,"
                       "Email VARCHAR(30) NOT NULL,"
                       "PRIMARY KEY(group_id, Email),"
                       "FOREIGN KEY(group_id) REFERENCES Group_table(group_id),"
                       "FOREIGN KEY(Email) REFERENCES User(email));";

    // Create Request table
    char *sql_request = "CREATE TABLE IF NOT EXISTS Request ("
                        "request_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "email VARCHAR(30) NOT NULL,"
                        "group_id INTEGER NOT NULL,"
                        "status VARCHAR(30) DEFAULT 'pending',"
                        "FOREIGN KEY(email) REFERENCES User(email),"
                        "FOREIGN KEY(group_id) REFERENCES Group_table(group_id));";

    // Create Invitation table
    char *sql_invitation = "CREATE TABLE IF NOT EXISTS Invitation ("
                           "invite_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "sender_email VARCHAR(30) NOT NULL,"
                           "receiver_email VARCHAR(30) NOT NULL,"
                           "group_id INTEGER NOT NULL,"
                           "status VARCHAR(30) DEFAULT 'pending',"
                           "FOREIGN KEY(sender_email) REFERENCES User(email),"
                           "FOREIGN KEY(receiver_email) REFERENCES User(email),"
                           "FOREIGN KEY(group_id) REFERENCES Group_table(group_id));";

    char *err_msg = 0;
    rc = sqlite3_exec(db, sql_user, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error (User): %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_exec(db, sql_group, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error (Group): %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_exec(db, sql_member, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error (Member): %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_exec(db, sql_request, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error (Request): %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    rc = sqlite3_exec(db, sql_invitation, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error (Invitation): %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    return 0;
}

int create_group(const char *group_name, const char *leader_email)
{
    char *sql = "INSERT INTO Group_table (name, leader) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, leader_email, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Failed to create group: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Add leader as member
    int group_id = (int)sqlite3_last_insert_rowid(db);
    add_member(group_id, leader_email);

    // Create folder for group
    char folder_path[256];
    snprintf(folder_path, sizeof(folder_path), "Group_folders/%s", group_name);
    mkdir(folder_path, 0755);

    return group_id;
}

int list_groups(char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';
    char *sql = "SELECT g.group_id, g.name, COUNT(m.Email) as member_count "
                "FROM Group_table g "
                "LEFT JOIN Member m ON g.group_id = m.group_id "
                "GROUP BY g.group_id, g.name;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    size_t offset = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int group_id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        int member_count = sqlite3_column_int(stmt, 2);

        int written = snprintf(buffer + offset, buffer_size - offset, "%d|%s|%d\n", group_id,
                               name, member_count);
        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
            break;
        }
        offset += written;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_group_id_by_name(const char *group_name)
{
    char *sql = "SELECT group_id FROM Group_table WHERE name = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, group_name, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    int group_id = -1;
    if (rc == SQLITE_ROW)
    {
        group_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return group_id;
}

int add_member(int group_id, const char *email)
{
    char *sql = "INSERT OR IGNORE INTO Member (group_id, Email) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int remove_member(int group_id, const char *email)
{
    char *sql = "DELETE FROM Member WHERE group_id = ? AND Email = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int list_members(int group_id, char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';
    char *sql = "SELECT Email FROM Member WHERE group_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);

    size_t offset = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char *email = (const char *)sqlite3_column_text(stmt, 0);
        int written = snprintf(buffer + offset, buffer_size - offset, "%s\n", email);
        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
            break;
        }
        offset += written;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_member_count(int group_id)
{
    char *sql = "SELECT COUNT(*) FROM Member WHERE group_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    rc = sqlite3_step(stmt);

    int count = 0;
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int is_member(int group_id, const char *email)
{
    char *sql = "SELECT COUNT(*) FROM Member WHERE group_id = ? AND Email = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    int count = 0;
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

int is_leader(int group_id, const char *email)
{
    char *sql = "SELECT COUNT(*) FROM Group_table WHERE group_id = ? AND leader = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    int count = 0;
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

int create_request(const char *email, int group_id)
{
    char *sql = "INSERT OR IGNORE INTO Request (email, group_id, status) VALUES (?, ?, 'pending');";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, group_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int approve_request(int request_id)
{
    // Get request details
    char *sql_get = "SELECT email, group_id FROM Request WHERE request_id = ? AND status = 'pending';";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql_get, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, request_id);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    const char *email = (const char *)sqlite3_column_text(stmt, 0);
    int group_id = sqlite3_column_int(stmt, 1);
    sqlite3_finalize(stmt);

    // Check if user is already a member
    if (is_member(group_id, email))
    {
        // Already a member, just update request status
        char *sql_update = "UPDATE Request SET status = 'approved' WHERE request_id = ?;";
        rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, request_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        return 0;
    }

    // Add member
    if (add_member(group_id, email) != 0)
    {
        return -1;
    }

    // Update request status
    char *sql_update = "UPDATE Request SET status = 'approved' WHERE request_id = ?;";
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, request_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int reject_request(int request_id)
{
    char *sql = "UPDATE Request SET status = 'rejected' WHERE request_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, request_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int list_requests(int group_id, char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';
    char *sql = "SELECT request_id, email FROM Request WHERE group_id = ? AND status = 'pending';";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);

    size_t offset = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int request_id = sqlite3_column_int(stmt, 0);
        const char *email = (const char *)sqlite3_column_text(stmt, 1);
        int written = snprintf(buffer + offset, buffer_size - offset, "%d|%s\n", request_id, email);
        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
            break;
        }
        offset += written;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int get_request_id(const char *email, int group_id)
{
    char *sql = "SELECT request_id FROM Request WHERE email = ? AND group_id = ? AND status = 'pending';";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, group_id);
    rc = sqlite3_step(stmt);

    int request_id = -1;
    if (rc == SQLITE_ROW)
    {
        request_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return request_id;
}

int get_request_group_id(int request_id)
{
    char *sql = "SELECT group_id FROM Request WHERE request_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, request_id);
    rc = sqlite3_step(stmt);

    int group_id = -1;
    if (rc == SQLITE_ROW)
    {
        group_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return group_id;
}

int user_exists(const char *email)
{
    char *sql = "SELECT COUNT(*) FROM User WHERE email = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    int count = 0;
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count > 0;
}

int create_user(const char *email, const char *password, const char *username)
{
    char *sql = "INSERT INTO User (email, password, username) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int verify_login(const char *email, const char *password, char *username, size_t username_size)
{
    char *sql = "SELECT username FROM User WHERE email = ? AND password = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        const char *db_username = (const char *)sqlite3_column_text(stmt, 0);
        if (db_username)
        {
            strncpy(username, db_username, username_size - 1);
            username[username_size - 1] = '\0';
        }
        sqlite3_finalize(stmt);
        return 0;  // Login successful
    }

    sqlite3_finalize(stmt);
    return -1;  // Login failed
}

int register_user(const char *email, const char *password, const char *username)
{
    // Check if user already exists
    if (user_exists(email))
    {
        return -1;  // User already exists
    }

    // Create new user
    return create_user(email, password, username);
}

