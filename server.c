#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sodium.h>
#include <stdint.h>  // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "database.h"

// Access database functions directly (database.c uses static db)
extern int is_leader(int group_id, const char *email);
extern int is_member(int group_id, const char *email);

#define MAX_FILENAME 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 20

struct sockaddr_in server_addr, client_addr;
int s, c;
// char BASE_PATH[MAX_PATH_LEN] = "Group_folders/";
// char *root_path = "Group_folders/";
// char copied_path[MAX_PATH_LEN + MAX_FILENAME];

void writeLog(const char *function_name, const char *user, const char *status, const char *details)
{
    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL)
    {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL)
    {
        perror("localtime failed");
        fclose(log_file);
        return;
    }

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Use fputs to avoid interpreting format specifiers in user strings
    fputs("[", log_file);
    fputs(timestamp, log_file);
    fputs("] [", log_file);
    fputs(function_name, log_file);
    fputs("] [", log_file);
    fputs(user, log_file);
    fputs("] [", log_file);
    fputs(status, log_file);
    fputs("] ", log_file);
    fputs(details, log_file);
    fputs("\n", log_file);
    fclose(log_file);
}

void generate_random_token(char *buffer, int length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length - 1; i++)
    {
        // Sử dụng randombytes_uniform của libsodium để an toàn hơn rand()
        // Nếu không thích dùng libsodium ở đây, có thể dùng rand() % ...
        int key = randombytes_uniform((uint32_t)(sizeof(charset) - 1));
        buffer[i] = charset[key];
    }
    buffer[length - 1] = '\0';
}

void createGroup(int c)
{
    char group_name[MAX_FILENAME];
    char leader_email[MAX_EMAIL_LEN];
    int bytes_recv;

    bytes_recv = recv(c, group_name, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_name");
        return;
    }
    group_name[bytes_recv] = '\0';

    bytes_recv = recv(c, leader_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv leader_email");
        return;
    }
    leader_email[bytes_recv] = '\0';

    int group_id = create_group(group_name, leader_email);
    if (group_id > 0)
    {
        char response[32];
        snprintf(response, sizeof(response), "OK|%d", group_id);
        send(c, response, sizeof(response), 0);
        printf("Successfully created group: %s (ID: %d)\n", group_name, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Created group '%s' with ID %d", group_name,
                 group_id);
        writeLog("CREATE_GROUP", leader_email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to create group: %s\n", group_name);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to create group '%s'", group_name);
        writeLog("CREATE_GROUP", leader_email, "ERROR", log_details);
    }
}

void listGroups(int c)
{
    char buffer[8192];
    if (list_groups(buffer, sizeof(buffer)) == 0)
    {
        int len = strlen(buffer);
        send(c, buffer, len, 0);
        printf("Sent group list to client\n");
    }
    else
    {
        send(c, "ERROR", 6, 0);
    }
}

void listMembers(int c)
{
    char group_id_str[32];
    int bytes_recv = recv(c, group_id_str, sizeof(group_id_str), 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    char buffer[4096];
    if (list_members(group_id, buffer, sizeof(buffer)) == 0)
    {
        int len = strlen(buffer);
        send(c, buffer, len, 0);
        printf("Sent member list for group %d to client\n", group_id);
    }
    else
    {
        send(c, "ERROR", 6, 0);
    }
}

void requestJoin(int c)
{
    char email[MAX_EMAIL_LEN];
    char group_id_str[32];
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';

    bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    printf("%d\n", bytes_recv);
    printf("%s\n", email);
    printf("%s\n", group_id_str);
    int group_id = atoi(group_id_str);

    if (create_request(email, group_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Request created: %s -> group %d\n", email, group_id);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to create request: %s -> group %d\n", email, group_id);
    }
}

void approveRequest(int c)
{
    char request_id_str[MAX_FILENAME];
    char email[MAX_EMAIL_LEN];
    int bytes_recv = recv(c, request_id_str, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv request_id");
        return;
    }
    request_id_str[bytes_recv] = '\0';
    int request_id = atoi(request_id_str);

    // Get email to verify leader
    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    // int email_len = strlen(email);
    // for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] ==
    // '\n');
    //      i--)
    // {
    //     email[i] = '\0';
    // }

    // Get group_id from request to check if user is leader
    int group_id = get_request_group_id(request_id);

    // Verify user is leader
    if (group_id > 0 && !is_leader(group_id, email))
    {
        send(c, "ERROR|NOT_LEADER", 16, 0);
        printf("User %s is not leader of group %d, cannot approve request %d\n", email, group_id,
               request_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details),
                 "Not leader of group %d, cannot approve request %d", group_id, request_id);
        writeLog("APPROVE_REQUEST", email, "ERROR", log_details);
        return;
    }

    if (approve_request(request_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Request %d approved by %s\n", request_id, email);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Approved request %d for group %d", request_id,
                 group_id);
        writeLog("APPROVE_REQUEST", email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to approve request %d\n", request_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to approve request %d", request_id);
        writeLog("APPROVE_REQUEST", email, "ERROR", log_details);
    }
}

void listRequests(int c)
{
    char group_id_str[32];
    int bytes_recv = recv(c, group_id_str, sizeof(group_id_str), 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    char buffer[4096];
    if (list_requests(group_id, buffer, sizeof(buffer)) == 0)
    {
        int len = strlen(buffer);
        send(c, buffer, len, 0);
        printf("Sent request list for group %d to client\n", group_id);
    }
    else
    {
        send(c, "ERROR", 6, 0);
    }
}

void rejectRequest(int c)
{
    char request_id_str[32];
    char email[MAX_EMAIL_LEN];
    int bytes_recv = recv(c, request_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv request_id");
        return;
    }
    request_id_str[bytes_recv] = '\0';
    int request_id = atoi(request_id_str);

    // Get email to verify leader
    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';

    // Get group_id from request to check if user is leader
    int group_id = get_request_group_id(request_id);

    // Verify user is leader
    if (group_id > 0 && !is_leader(group_id, email))
    {
        send(c, "ERROR|NOT_LEADER", 16, 0);
        printf("User %s is not leader of group %d, cannot reject request %d\n", email, group_id,
               request_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details),
                 "Not leader of group %d, cannot reject request %d", group_id, request_id);
        writeLog("REJECT_REQUEST", email, "ERROR", log_details);
        return;
    }

    if (reject_request(request_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Request %d rejected by %s\n", request_id, email);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Rejected request %d for group %d", request_id,
                 group_id);
        writeLog("REJECT_REQUEST", email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to reject request %d\n", request_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to reject request %d", request_id);
        writeLog("REJECT_REQUEST", email, "ERROR", log_details);
    }
}

void removeMember(int c)
{
    char leader_email[MAX_EMAIL_LEN];
    char member_email[MAX_EMAIL_LEN];
    char group_id_str[32];
    int bytes_recv;

    bytes_recv = recv(c, leader_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv leader_email");
        return;
    }
    leader_email[bytes_recv] = '\0';

    bytes_recv = recv(c, member_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv member_email");
        return;
    }
    member_email[bytes_recv] = '\0';

    bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    printf("Remove request: leader=%s, member=%s, group=%d\n", leader_email, member_email,
           group_id);

    // Verify user is leader
    if (!is_leader(group_id, leader_email))
    {
        send(c, "ERROR|NOT_LEADER", 16, 0);
        printf("User %s is not leader of group %d, cannot remove members\n", leader_email,
               group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details),
                 "Not leader of group %d, cannot remove member %s", group_id, member_email);
        writeLog("REMOVE_MEMBER", leader_email, "ERROR", log_details);
        return;
    }

    // Cannot remove leader
    if (is_leader(group_id, member_email))
    {
        send(c, "ERROR|IS_LEADER", 16, 0);
        printf("Cannot remove leader %s from group %d\n", member_email, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Cannot remove leader %s from group %d",
                 member_email, group_id);
        writeLog("REMOVE_MEMBER", leader_email, "ERROR", log_details);
        return;
    }

    if (remove_member(group_id, member_email) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Leader %s removed member %s from group %d\n", leader_email, member_email, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Removed member %s from group %d", member_email,
                 group_id);
        writeLog("REMOVE_MEMBER", leader_email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to remove member %s from group %d\n", member_email, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to remove member %s from group %d",
                 member_email, group_id);
        writeLog("REMOVE_MEMBER", leader_email, "ERROR", log_details);
    }
}

void leaveGroup(int c)
{
    char email[MAX_EMAIL_LEN];
    char group_id_str[32];
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';

    bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    printf("%s\n", group_id_str);
    printf("%s\n", email);
    int group_id = atoi(group_id_str);

    // Check if user is leader
    if (is_leader(group_id, email))
    {
        send(c, "ERROR|LEADER", 13, 0);
        printf("Cannot remove leader from group %d\n", group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Cannot leave group %d - user is leader",
                 group_id);
        writeLog("LEAVE_GROUP", email, "ERROR", log_details);
        return;
    }

    if (remove_member(group_id, email) == 0)
    {
        send(c, "OK", 3, 0);
        printf("User %s left group %d\n", email, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Left group %d", group_id);
        writeLog("LEAVE_GROUP", email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to remove user %s from group %d\n", email, group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to leave group %d", group_id);
        writeLog("LEAVE_GROUP", email, "ERROR", log_details);
    }
}

void loginUser(int c)
{
    char email[MAX_EMAIL_LEN];
    char password_buf[MAX_FILENAME];  // Match client's MAX_FILENAME
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    // Trim trailing spaces/null bytes
    // int email_len = strlen(email);
    // for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] ==
    // '\n');
    //      i--)
    // {
    //     email[i] = '\0';
    // }

    bytes_recv = recv(c, password_buf, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv password");
        return;
    }
    password_buf[bytes_recv] = '\0';

    // Extract actual password (trim null bytes and spaces)
    char password[32] = {0};
    int j = 0;
    for (int i = 0; i < bytes_recv && j < 31; i++)
    {
        if (password_buf[i] != '\0' && password_buf[i] != ' ' && password_buf[i] != '\n')
        {
            password[j++] = password_buf[i];
        }
        else if (j > 0)  // Stop at first null/space after content
        {
            break;
        }
    }
    password[j] = '\0';

    printf("Login attempt: email='%s', password='%s' (len=%d)\n", email, password,
           (int)strlen(password));

    char username[MAX_USERNAME_LEN];
    if (verify_login(email, password, username, sizeof(username)) == 0)
    {
        // Generate random token
        char token[65];  // 64 characters + null terminator
        generate_random_token(token, 65);

        // Save session to database
        if (create_session(token, email, username) == 0)
        {
            char response[256];
            snprintf(response, sizeof(response), "OK|%s|%s|%s", email, username, token);
            send(c, response, strlen(response) + 1, 0);
            printf("User %s logged in successfully, token: %s\n", email, token);
            writeLog("LOGIN", email, "SUCCESS", "User logged in successfully");
        }
        else
        {
            send(c, "ERROR", 6, 0);
            printf("Failed to create session for %s\n", email);
            writeLog("LOGIN", email, "ERROR", "Failed to create session");
        }
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Login failed for %s (password: '%s')\n", email, password);
        writeLog("LOGIN", email, "ERROR", "Invalid credentials");
    }
}

void registerUser(int c)
{
    char email[MAX_EMAIL_LEN];
    char password_buf[MAX_FILENAME];
    char username_buf[MAX_FILENAME];
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    printf("%s\n", email);
    int email_len = strlen(email);
    for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] == '\n');
         i--)
    {
        email[i] = '\0';
    }
    printf("%s\n", email);

    bytes_recv = recv(c, password_buf, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv password");
        return;
    }
    password_buf[bytes_recv] = '\0';
    printf("%s\n", password_buf);
    char password[32] = {0};
    int j = 0;
    for (int i = 0; i < bytes_recv && j < 31; i++)
    {
        if (password_buf[i] != '\0' && password_buf[i] != ' ' && password_buf[i] != '\n')
        {
            password[j++] = password_buf[i];
        }
        else if (j > 0)
        {
            break;
        }
    }
    password[j] = '\0';
    printf("%s\n", password);

    bytes_recv = recv(c, username_buf, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv username");
        return;
    }
    username_buf[bytes_recv] = '\0';
    printf("%s\n", username_buf);
    char username[MAX_USERNAME_LEN] = {0};
    j = 0;
    for (int i = 0; i < bytes_recv && j < MAX_USERNAME_LEN - 1; i++)
    {
        if (username_buf[i] != '\0' && username_buf[i] != ' ' && username_buf[i] != '\n')
        {
            username[j++] = username_buf[i];
        }
        else if (j > 0)
        {
            break;
        }
    }
    username[j] = '\0';
    printf("%s\n", username);

    printf("Register attempt: email='%s', username='%s'\n", email, username);

    if (register_user(email, password, username_buf) == 0)
    {
        char response[256];
        snprintf(response, sizeof(response), "OK|%s|%s", email, username);
        send(c, response, strlen(response) + 1, 0);
        printf("User %s registered successfully\n", email);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "New user registered with username '%s'",
                 username);
        writeLog("REGISTER", email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR|EXISTS", 13, 0);
        printf("Registration failed for %s (user may already exist)\n", email);
        writeLog("REGISTER", email, "ERROR", "Registration failed - user may already exist");
    }
}

void checkLeader(int c)
{
    char group_id_str[MAX_FILENAME];
    char email[MAX_EMAIL_LEN];
    int bytes_recv;

    bytes_recv = recv(c, group_id_str, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    // int email_len = strlen(email);
    // for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] ==
    // '\n');
    //      i--)
    // {
    //     email[i] = '\0';
    // }

    printf("%s\n", email);
    printf("%d\n", group_id);

    if (is_leader(group_id, email))
    {
        send(c, "YES", 4, 0);
    }
    else
    {
        send(c, "NO", 3, 0);
    }
}

void checkMember(int c)
{
    char group_id_str[MAX_FILENAME];
    char email[MAX_EMAIL_LEN];
    int bytes_recv;

    bytes_recv = recv(c, group_id_str, MAX_FILENAME, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';

    printf("Checking membership: %s in group %d\n", email, group_id);

    if (is_member(group_id, email))
    {
        send(c, "YES", 4, 0);
    }
    else
    {
        send(c, "NO", 3, 0);
    }
}

void listNonMembers(int c)
{
    char group_id_str[32];
    int bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    char buffer[8192];
    if (list_non_members(group_id, buffer, sizeof(buffer)) == 0)
    {
        int len = strlen(buffer);
        send(c, buffer, len, 0);
        printf("Sent non-member list for group %d to client\n", group_id);
    }
    else
    {
        send(c, "ERROR", 6, 0);
    }
}

void inviteUser(int c)
{
    char sender_email[MAX_EMAIL_LEN];
    char receiver_email[MAX_EMAIL_LEN];
    char group_id_str[32];
    int bytes_recv;

    bytes_recv = recv(c, sender_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv sender_email");
        return;
    }
    sender_email[bytes_recv] = '\0';

    bytes_recv = recv(c, receiver_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv receiver_email");
        return;
    }
    receiver_email[bytes_recv] = '\0';

    bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    if (create_invitation(sender_email, receiver_email, group_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Invitation created: %s invited %s to group %d\n", sender_email, receiver_email,
               group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Invited %s to group %d", receiver_email,
                 group_id);
        writeLog("INVITE_USER", sender_email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to create invitation: %s -> %s (group %d)\n", sender_email, receiver_email,
               group_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to invite %s to group %d",
                 receiver_email, group_id);
        writeLog("INVITE_USER", sender_email, "ERROR", log_details);
    }
}

void listInvitations(int c)
{
    char receiver_email[MAX_EMAIL_LEN];
    int bytes_recv = recv(c, receiver_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv receiver_email");
        return;
    }
    receiver_email[bytes_recv] = '\0';

    char buffer[4096];
    if (list_invitations(receiver_email, buffer, sizeof(buffer)) == 0)
    {
        int len = strlen(buffer);
        send(c, buffer, len, 0);
        printf("Sent invitation list for %s to client\n", receiver_email);
    }
    else
    {
        send(c, "ERROR", 6, 0);
    }
}

void acceptInvite(int c)
{
    char invite_id_str[32];
    int bytes_recv = recv(c, invite_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv invite_id");
        return;
    }
    invite_id_str[bytes_recv] = '\0';
    int invite_id = atoi(invite_id_str);

    if (accept_invitation(invite_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Invitation %d accepted\n", invite_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Accepted invitation %d", invite_id);
        writeLog("ACCEPT_INVITE", "unknown", "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to accept invitation %d\n", invite_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to accept invitation %d", invite_id);
        writeLog("ACCEPT_INVITE", "unknown", "ERROR", log_details);
    }
}

void checkPendingInvite(int c)
{
    char receiver_email[MAX_EMAIL_LEN];
    char group_id_str[32];
    int bytes_recv;

    bytes_recv = recv(c, receiver_email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv receiver_email");
        return;
    }
    receiver_email[bytes_recv] = '\0';

    bytes_recv = recv(c, group_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    int invite_id = get_pending_invitation_id(receiver_email, group_id);
    if (invite_id > 0)
    {
        char response[32];
        snprintf(response, sizeof(response), "OK|%d", invite_id);
        send(c, response, strlen(response), 0);
        printf("Found pending invitation %d for %s in group %d\n", invite_id, receiver_email,
               group_id);
    }
    else
    {
        send(c, "NONE", 5, 0);
        printf("No pending invitation for %s in group %d\n", receiver_email, group_id);
    }
}

void rejectInvite(int c)
{
    char invite_id_str[32];
    int bytes_recv = recv(c, invite_id_str, 32, 0);
    if (bytes_recv <= 0)
    {
        perror("recv invite_id");
        return;
    }
    invite_id_str[bytes_recv] = '\0';
    int invite_id = atoi(invite_id_str);

    if (reject_invitation(invite_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Invitation %d rejected\n", invite_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Rejected invitation %d", invite_id);
        writeLog("REJECT_INVITE", "unknown", "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to reject invitation %d\n", invite_id);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Failed to reject invitation %d", invite_id);
        writeLog("REJECT_INVITE", "unknown", "ERROR", log_details);
    }
}

void updateUsername(int c)
{
    char data[256];
    int bytes_recv = recv(c, data, sizeof(data) - 1, 0);
    if (bytes_recv <= 0)
    {
        perror("recv update username data");
        return;
    }
    data[bytes_recv] = '\0';

    // Parse: email|new_username
    char *email = strtok(data, "|");
    char *new_username = strtok(NULL, "|");

    if (!email || !new_username)
    {
        send(c, "ERROR", 6, 0);
        return;
    }

    if (update_username(email, new_username) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Username updated for %s to %s\n", email, new_username);
        char log_details[256];
        snprintf(log_details, sizeof(log_details), "Updated username to '%s'", new_username);
        writeLog("UPDATE_USERNAME", email, "SUCCESS", log_details);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to update username for %s\n", email);
        writeLog("UPDATE_USERNAME", email, "ERROR", "Failed to update username");
    }
}

void deleteUser(int c)
{
    char email[MAX_EMAIL_LEN];
    int bytes_recv = recv(c, email, sizeof(email) - 1, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';

    if (delete_user(email) == 0)
    {
        send(c, "OK", 3, 0);
        printf("User %s deleted successfully\n", email);
        writeLog("DELETE_USER", email, "SUCCESS", "User account deleted");
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to delete user %s\n", email);
        writeLog("DELETE_USER", email, "ERROR", "Failed to delete user account");
    }
}

void verifyToken(int c)
{
    char token[65];
    int bytes_recv = recv(c, token, 65, 0);
    if (bytes_recv <= 0)
    {
        perror("recv token");
        return;
    }
    token[bytes_recv] = '\0';

    char email[MAX_EMAIL_LEN];
    char username[MAX_USERNAME_LEN];
    char created_at[64];

    if (get_session_info(token, email, sizeof(email), username, sizeof(username), created_at,
                         sizeof(created_at)) == 0)
    {
        // Return: OK|email|username|created_at
        char response[512];
        snprintf(response, sizeof(response), "OK|%s|%s|%s", email, username, created_at);
        send(c, response, strlen(response) + 1, 0);
        printf("Token verified for %s, created at %s\n", email, created_at);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Invalid token\n");
    }
}

void handle_client(int c)
{
    int bytes_recv;
    char command[COMMAND_LENGTH];
    bytes_recv = recv(c, command, COMMAND_LENGTH, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    command[bytes_recv] = '\0';
    // Trim trailing whitespace and null bytes
    int cmd_len = strlen(command);
    for (int i = cmd_len - 1; i >= 0 && (command[i] == ' ' || command[i] == '\0' ||
                                         command[i] == '\n' || command[i] == '\r');
         i--)
    {
        command[i] = '\0';
    }
    printf("Received command: %s\n", command);
    if (strcmp(command, "CREATE_GROUP") == 0)
    {
        createGroup(c);
    }
    else if (strcmp(command, "LIST_GROUP") == 0)
    {
        listGroups(c);
    }
    else if (strcmp(command, "LIST_MEMBERS") == 0)
    {
        listMembers(c);
    }
    else if (strcmp(command, "REQUEST_JOIN") == 0)
    {
        requestJoin(c);
    }
    else if (strcmp(command, "APPROVE_REQ") == 0)
    {
        approveRequest(c);
    }
    else if (strcmp(command, "REJECT_REQUEST") == 0)
    {
        rejectRequest(c);
    }
    else if (strcmp(command, "LIST_REQUESTS") == 0)
    {
        listRequests(c);
    }
    else if (strcmp(command, "LEAVE_GROUP") == 0)
    {
        leaveGroup(c);
    }
    else if (strcmp(command, "REMOVE_MEMBER") == 0)
    {
        removeMember(c);
    }
    else if (strcmp(command, "LOGIN") == 0)
    {
        loginUser(c);
    }
    else if (strcmp(command, "REGISTER") == 0)
    {
        registerUser(c);
    }
    else if (strcmp(command, "CHECK_LEADER") == 0)
    {
        checkLeader(c);
    }
    else if (strcmp(command, "CHECK_MEMBER") == 0)
    {
        checkMember(c);
    }
    else if (strcmp(command, "NON_MEMBERS") == 0)
    {
        listNonMembers(c);
    }
    else if (strcmp(command, "INVITE_USER") == 0)
    {
        inviteUser(c);
    }
    else if (strcmp(command, "LIST_INVITES") == 0)
    {
        listInvitations(c);
    }
    else if (strcmp(command, "ACCEPT_INVITE") == 0)
    {
        acceptInvite(c);
    }
    else if (strcmp(command, "CHECK_INVITE") == 0)
    {
        checkPendingInvite(c);
    }
    else if (strcmp(command, "REJECT_INVITE") == 0)
    {
        rejectInvite(c);
    }
    else if (strcmp(command, "UPDATE_USERNAME") == 0)
    {
        updateUsername(c);
    }
    else if (strcmp(command, "DELETE_USER") == 0)
    {
        deleteUser(c);
    }
    else if (strcmp(command, "VERIFY_TOKEN") == 0)
    {
        verifyToken(c);
    }
    else
    {
        printf("Unknown command: %s\n", command);
    }
}

// Thread function to handle client
void *client_thread(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    handle_client(client_sock);
    close(client_sock);

    return NULL;
}

int main()
{
    // Initialize database
    if (init_database() != 0)
    {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8081);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        perror("socket");
        exit(1);
    }

    if (bind(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) < 0)
    {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port 8081...\n");

    while (1)
    {
        socklen_t clen = sizeof(struct sockaddr);
        c = accept(s, (struct sockaddr *)&client_addr, &clen);
        if (c < 0)
        {
            perror("accept");
            continue;
        }

        // Allocate memory for socket descriptor to pass to thread
        int *client_sock = malloc(sizeof(int));
        if (client_sock == NULL)
        {
            perror("malloc");
            close(c);
            continue;
        }
        *client_sock = c;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, client_sock) != 0)
        {
            perror("pthread_create");
            free(client_sock);
            close(c);
            continue;
        }

        // Detach thread so it cleans up automatically when done
        pthread_detach(thread_id);
    }
}
