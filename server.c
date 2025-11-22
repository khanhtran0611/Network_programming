#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>  // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "database.h"

// Access database functions directly (database.c uses static db)
extern int is_leader(int group_id, const char *email);

#define MAX_FILE_LEN 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 16

struct sockaddr_in server_addr, client_addr;
int s, c;
char BASE_PATH[MAX_PATH_LEN] = "Group_folders/";
char *root_path = "Group_folders/";
char copied_path[MAX_PATH_LEN + MAX_FILE_LEN];

void addFile(int c)
{
    char filename[MAX_FILE_LEN];
    char buffer[10240];
    int bytes_recv = recv(c, filename, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    char full_path[MAX_PATH_LEN + MAX_FILE_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, filename);
    FILE *fp = fopen(full_path, "wb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(1);
    }
    while ((bytes_recv = recv(c, buffer, 10240, 0)) > 0)
    {
        fwrite(buffer, 1, bytes_recv, fp);
    }
    fclose(fp);  // Di chuyển fclose() lên trước khi kiểm tra lỗi
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    printf("Sucessfully uploaded file : %s\n", filename);
}

void viewFolder(int c, char *full_path)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(full_path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                // Send filename
                if (send(c, dir->d_name, MAX_FILE_LEN, 0) < 0)
                {
                    perror("send filename");
                    exit(1);
                }
                printf("%s\n", dir->d_name);
                char *type;
                if (dir->d_type == DT_DIR)
                {
                    type = "folder";
                }
                else
                {
                    type = "file";
                }
                printf("%s\n", type);
                // Send type
                if (send(c, type, 10, 0) < 0)
                {
                    perror("send type");
                    exit(1);
                }
            }
        }
        closedir(d);
        // Signal end of list by sending a zero length for the next item
        uint32_t zero_len = htonl(0);
        if (send(c, &zero_len, MAX_FILE_LEN, 0) < 0)
        {
            perror("send zero_len");
            exit(1);
        }
        if (shutdown(c, SHUT_WR) < 0)
        {
            perror("shutdown");
        }
    }
    else
    {
        perror("opendir");
        exit(1);
    }
}

void get_list_of_files(int c)
{
    char foldername[MAX_FILE_LEN];
    int bytes_recv = recv(c, foldername, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    foldername[bytes_recv] = '\0';
    strncat(BASE_PATH, foldername, sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    strncat(BASE_PATH, "/", sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    viewFolder(c, BASE_PATH);
}

void back(int c)
{
    char *temp = dirname(BASE_PATH);
    strcpy(BASE_PATH, temp);
    strcat(BASE_PATH, "/");
    printf("%s\n", BASE_PATH);
    viewFolder(c, BASE_PATH);
}

void deleteFile(int c)
{
    char filename[MAX_FILE_LEN];
    int bytes_recv = recv(c, filename, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    char full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, filename);
    printf("%s\n", filename);
    if (remove(full_path) != 0)
    {
        perror("remove");
        exit(1);
    }
    printf("Sucessfully deleted file : %s\n", filename);
}

void addFolder(int c)
{
    char foldername[MAX_FILE_LEN];
    int bytes_recv = recv(c, foldername, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        return;  // Don't exit the whole server
    }
    foldername[bytes_recv] = '\0';
    char full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, foldername);
    if (mkdir(full_path, 0755) == 0)
    {  // Use 0755 for standard permissions
        printf("Successfully created folder: %s\n", foldername);
    }
    else
    {
        perror("mkdir");
    }
}

void deleteFolder(int c)
{
    char foldername[MAX_FILE_LEN];
    int bytes_recv = recv(c, foldername, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        return;  // Don't exit the whole server
    }
    foldername[bytes_recv] = '\0';
    char full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, foldername);
    char command[MAX_FILE_LEN + MAX_PATH_LEN + 16];
    strcpy(command, "rm -rf ");
    strcat(command, full_path);
    if (system(command) == 0)
    {
        printf("Successfully deleted folder: %s\n", foldername);
    }
    else
    {
        perror("system");
    }
}

void renameFileOrFolder(int c)
{
    char old_name[MAX_FILE_LEN];
    char new_name[MAX_FILE_LEN];
    int bytes_recv;

    // Receive old name
    bytes_recv = recv(c, old_name, MAX_FILE_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv old_name");
        return;
    }
    old_name[bytes_recv] = '\0';
    printf("Received old name: %s\n", old_name);

    // Receive new name
    bytes_recv = recv(c, new_name, MAX_FILE_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv new_name");
        return;
    }
    printf("Received new name: %s\n", new_name);
    new_name[bytes_recv] = '\0';

    char old_full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    char new_full_path[MAX_FILE_LEN + MAX_PATH_LEN];

    snprintf(old_full_path, sizeof(old_full_path), "%s%s", BASE_PATH, old_name);
    snprintf(new_full_path, sizeof(new_full_path), "%s%s", BASE_PATH, new_name);

    if (rename(old_full_path, new_full_path) == 0)
    {
        printf("Successfully renamed '%s' to '%s'\n", old_name, new_name);
    }
    else
    {
        perror("rename");
    }
}

void download(int c)
{
    printf("1\n");
    char filename[MAX_FILE_LEN];
    int bytes_recv = recv(c, filename, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    char full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, filename);
    printf("%s\n", full_path);
    FILE *fp = fopen(full_path, "rb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(1);
    }
    char buffer[10240];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        printf("Read %d bytes \n", bytes_read);
        if (send(c, buffer, bytes_read, 0) < 0)
        {
            perror("send");
            exit(1);
        }
    }
    if (shutdown(c, SHUT_WR) < 0)
    {
        perror("shutdown");
    }
    fclose(fp);
}

void out(int c)
{
    strcpy(BASE_PATH, root_path);
}

void copy(int c)
{
    char filename[MAX_FILE_LEN];
    int bytes_recv = recv(c, filename, MAX_FILE_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    strcpy(copied_path, BASE_PATH);
    strcat(copied_path, filename);
    printf("%s\n", copied_path);
}

void paste(int c) {}

void createGroup(int c)
{
    char group_name[MAX_FILE_LEN];
    char leader_email[MAX_EMAIL_LEN];
    int bytes_recv;

    bytes_recv = recv(c, group_name, MAX_FILE_LEN, 0);
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
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to create group: %s\n", group_name);
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

    bytes_recv = recv(c, group_id_str, sizeof(group_id_str), 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
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
    char request_id_str[32];
    char email[MAX_EMAIL_LEN];
    int bytes_recv = recv(c, request_id_str, sizeof(request_id_str), 0);
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
    int email_len = strlen(email);
    for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] == '\n'); i--)
    {
        email[i] = '\0';
    }

    // Get group_id from request to check if user is leader
    int group_id = get_request_group_id(request_id);

    // Verify user is leader
    if (group_id > 0 && !is_leader(group_id, email))
    {
        send(c, "ERROR|NOT_LEADER", 16, 0);
        printf("User %s is not leader of group %d, cannot approve request %d\n", email, group_id,
               request_id);
        return;
    }

    if (approve_request(request_id) == 0)
    {
        send(c, "OK", 3, 0);
        printf("Request %d approved by %s\n", request_id, email);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to approve request %d\n", request_id);
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

    bytes_recv = recv(c, group_id_str, sizeof(group_id_str), 0);
    if (bytes_recv <= 0)
    {
        perror("recv group_id");
        return;
    }
    group_id_str[bytes_recv] = '\0';
    int group_id = atoi(group_id_str);

    // Check if user is leader
    if (is_leader(group_id, email))
    {
        send(c, "ERROR|LEADER", 13, 0);
        printf("Cannot remove leader from group %d\n", group_id);
        return;
    }

    if (remove_member(group_id, email) == 0)
    {
        send(c, "OK", 3, 0);
        printf("User %s left group %d\n", email, group_id);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Failed to remove user %s from group %d\n", email, group_id);
    }
}

void loginUser(int c)
{
    char email[MAX_EMAIL_LEN];
    char password_buf[MAX_FILE_LEN];  // Match client's MAX_FILENAME
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    // Trim trailing spaces/null bytes
    int email_len = strlen(email);
    for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] == '\n'); i--)
    {
        email[i] = '\0';
    }

    bytes_recv = recv(c, password_buf, MAX_FILE_LEN, 0);
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

    printf("Login attempt: email='%s', password='%s' (len=%d)\n", email, password, (int)strlen(password));

    char username[MAX_USERNAME_LEN];
    if (verify_login(email, password, username, sizeof(username)) == 0)
    {
        char response[256];
        snprintf(response, sizeof(response), "OK|%s|%s", email, username);
        send(c, response, strlen(response) + 1, 0);
        printf("User %s logged in successfully\n", email);
    }
    else
    {
        send(c, "ERROR", 6, 0);
        printf("Login failed for %s (password: '%s')\n", email, password);
    }
}

void registerUser(int c)
{
    char email[MAX_EMAIL_LEN];
    char password_buf[MAX_FILE_LEN];
    char username_buf[MAX_FILE_LEN];
    int bytes_recv;

    bytes_recv = recv(c, email, MAX_EMAIL_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv email");
        return;
    }
    email[bytes_recv] = '\0';
    int email_len = strlen(email);
    for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] == '\n'); i--)
    {
        email[i] = '\0';
    }

    bytes_recv = recv(c, password_buf, MAX_FILE_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv password");
        return;
    }
    password_buf[bytes_recv] = '\0';
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

    bytes_recv = recv(c, username_buf, MAX_FILE_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv username");
        return;
    }
    username_buf[bytes_recv] = '\0';
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

    printf("Register attempt: email='%s', username='%s'\n", email, username);

    if (register_user(email, password, username) == 0)
    {
        char response[256];
        snprintf(response, sizeof(response), "OK|%s|%s", email, username);
        send(c, response, strlen(response) + 1, 0);
        printf("User %s registered successfully\n", email);
    }
    else
    {
        send(c, "ERROR|EXISTS", 13, 0);
        printf("Registration failed for %s (user may already exist)\n", email);
    }
}

void checkLeader(int c)
{
    char group_id_str[32];
    char email[MAX_EMAIL_LEN];
    int bytes_recv;

    bytes_recv = recv(c, group_id_str, sizeof(group_id_str), 0);
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
    int email_len = strlen(email);
    for (int i = email_len - 1; i >= 0 && (email[i] == ' ' || email[i] == '\0' || email[i] == '\n'); i--)
    {
        email[i] = '\0';
    }

    if (is_leader(group_id, email))
    {
        send(c, "YES", 4, 0);
    }
    else
    {
        send(c, "NO", 3, 0);
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
    for (int i = cmd_len - 1; i >= 0 && (command[i] == ' ' || command[i] == '\0' || command[i] == '\n' || command[i] == '\r'); i--)
    {
        command[i] = '\0';
    }
    printf("Received command: %s\n", command);
    if (strcmp(command, "addFile") == 0)
    {
        addFile(c);
    }
    else if (strcmp(command, "getList") == 0)
    {
        get_list_of_files(c);
    }
    else if (strcmp(command, "deleteFile") == 0)
    {
        deleteFile(c);
    }
    else if (strcmp(command, "addFolder") == 0)
    {
        addFolder(c);
    }
    else if (strcmp(command, "deleteFolder") == 0)
    {
        deleteFolder(c);
    }
    else if (strcmp(command, "rename") == 0)
    {
        renameFileOrFolder(c);
    }
    else if (strcmp(command, "back") == 0)
    {
        back(c);
    }
    else if (strcmp(command, "download") == 0)
    {
        download(c);
    }
    else if (strcmp(command, "out") == 0)
    {
        out(c);
    }
    else if (strcmp(command, "copy") == 0)
    {
    }
    else if (strcmp(command, "paste") == 0)
    {
    }
    else if (strcmp(command, "CREATE_GROUP") == 0)
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
    else if (strcmp(command, "LIST_REQUESTS") == 0)
    {
        listRequests(c);
    }
    else if (strcmp(command, "LEAVE_GROUP") == 0)
    {
        leaveGroup(c);
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
    else
    {
        printf("Unknown command: %s\n", command);
    }
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
    server_addr.sin_port = htons(8080);
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
    while (1)
    {
        socklen_t clen = sizeof(struct sockaddr);
        c = accept(s, (struct sockaddr *)&client_addr, &clen);
        if (c < 0)
        {
            perror("accept");
            exit(1);
        }
        handle_client(c);
        close(c);
    }
}
