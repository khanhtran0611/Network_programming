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
    char full_path[MAX_FILE_LEN + MAX_PATH_LEN];
    strcpy(copied_path, BASE_PATH);
    strcat(copied_path, filename);
    printf("%s\n", copied_path);
}

void paste(int c) {}

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
    else
    {
        printf("Unknown command: %s\n", command);
    }
}

int main()
{
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
        int clen = sizeof(struct sockaddr);
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
