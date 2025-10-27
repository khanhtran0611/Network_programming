#define _DEFAULT_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdint.h> // For uint32_t
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <dirent.h>

struct sockaddr_in server_addr, client_addr;
int s, c;
const char *BASE_PATH = "Group_folders/GroupA/";

void addFile(int c)
{
    char filename[1024];
    char buffer[10240];
    int bytes_recv = recv(c, filename, 1024, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    char full_path[512];
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
    fclose(fp); // Di chuyển fclose() lên trước khi kiểm tra lỗi
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    printf("Sucessfully uploaded file : %s\n", filename);
}

void get_list_of_files(int c)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(BASE_PATH);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                // Send filename length
                uint32_t name_len = strlen(dir->d_name) + 1; // Include null terminator
                uint32_t net_name_len = htonl(name_len);
                if (send(c, &net_name_len, sizeof(net_name_len), 0) < 0)
                {
                    perror("send name_len");
                    exit(1);
                }
                // Send filename
                if (send(c, dir->d_name, name_len, 0) < 0)
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
                // Send type length
                uint32_t type_len = strlen(type) + 1; // Include null terminator
                uint32_t net_type_len = htonl(type_len);
                if (send(c, &net_type_len, sizeof(net_type_len), 0) < 0)
                {
                    perror("send type_len");
                    exit(1);
                }
                // Send type
                if (send(c, type, type_len, 0) < 0)
                {
                    perror("send type");
                    exit(1);
                }
            }
        }
        closedir(d);
        // Signal end of list by sending a zero length for the next item
        uint32_t zero_len = htonl(0);
        if (send(c, &zero_len, sizeof(zero_len), 0) < 0)
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

void deleteFile(int c)
{
    char filename[1024];
    char buffer[10240];
    int bytes_recv = recv(c, filename, 1024, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    filename[bytes_recv] = '\0';
    char full_path[512];
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

void handle_client(int c)
{

    int bytes_recv;
    char command[1024];
    bytes_recv = recv(c, command, 1024, 0);
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
