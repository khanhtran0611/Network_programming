#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdint.h>  // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_FILENAME 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 20

struct sockaddr_in server_addr, client_addr;
int s, c;
char BASE_PATH[MAX_PATH_LEN] = "Group_folders/";
char *root_path = "Group_folders/";
// char copied_path[MAX_PATH_LEN + MAX_FILENAME];

void writeLog(const char *function_name, const char *status, const char *details)
{
    FILE *log_file = fopen("server2.log", "a");
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

    // Use fputs to avoid interpreting format specifiers in strings
    fputs("[", log_file);
    fputs(timestamp, log_file);
    fputs("] [", log_file);
    fputs(function_name, log_file);
    fputs("] [", log_file);
    fputs(status, log_file);
    fputs("] ", log_file);
    fputs(details, log_file);
    fputs("\n", log_file);
    fclose(log_file);
}

void addFile(int c)
{
    char full_path[MAX_PATH_LEN + MAX_FILENAME];
    char buffer[10240];
    int bytes_recv = recv(c, full_path, MAX_PATH_LEN + MAX_FILENAME, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    full_path[bytes_recv] = '\0';
    char temp[MAX_FILENAME + MAX_PATH_LEN];
    printf("%s\n", full_path);
    strcpy(temp, full_path);
    printf("%s\n", temp);
    char duplicate_signal[20];
    while (access(temp, F_OK) == 0)
    {
        strcpy(duplicate_signal, "duplicate");
        if (send(c, duplicate_signal, 20, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        bytes_recv = recv(c, temp, MAX_FILENAME + MAX_PATH_LEN, 0);
        if (bytes_recv < 0)
        {
            perror("recv");
            exit(1);
        }
        if (bytes_recv == 0)
        {
            return;
        }
    }
    strcpy(duplicate_signal, "ok");
    if (send(c, duplicate_signal, 20, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    strcpy(full_path, temp);

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
    printf("Sucessfully uploaded file : %s\n", full_path);
    char log_details[512];
    snprintf(log_details, sizeof(log_details), "File uploaded: %s", full_path);
    writeLog("ADD_FILE", "SUCCESS", log_details);
}

void viewFolder(int c, char *full_path)
{
    DIR *d;
    struct dirent *dir;
    printf("%s\n", full_path);
    d = opendir(full_path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                // Send filename
                if (send(c, dir->d_name, MAX_FILENAME, 0) < 0)
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
        if (send(c, &zero_len, MAX_FILENAME, 0) < 0)
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
    char folderpath[MAX_PATH_LEN];
    int bytes_recv = recv(c, folderpath, MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    folderpath[bytes_recv] = '\0';
    // strncat(BASE_PATH, foldername, sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    // strncat(BASE_PATH, "/", sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    viewFolder(c, folderpath);
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
    char full_path[MAX_FILENAME + MAX_PATH_LEN];
    int bytes_recv = recv(c, full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    full_path[bytes_recv] = '\0';

    if (remove(full_path) != 0)
    {
        perror("remove");
        char log_details[512];
        snprintf(log_details, sizeof(log_details), "Failed to delete file: %s", full_path);
        writeLog("DELETE_FILE", "ERROR", log_details);
        exit(1);
    }
    printf("Sucessfully deleted file : %s\n", full_path);
    char log_details[512];
    snprintf(log_details, sizeof(log_details), "File deleted: %s", full_path);
    writeLog("DELETE_FILE", "SUCCESS", log_details);
}

void addFolder(int c)
{
    char full_path[MAX_FILENAME + MAX_PATH_LEN];
    int bytes_recv = recv(c, full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        return;  // Don't exit the whole server
    }
    full_path[bytes_recv] = '\0';
    char temp[MAX_FILENAME + MAX_PATH_LEN];
    strcpy(temp, full_path);
    printf("%s\n", temp);
    char duplicate_signal[20];
    while (access(temp, F_OK) == 0)
    {
        strcpy(duplicate_signal, "duplicate");
        if (send(c, duplicate_signal, 20, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        bytes_recv = recv(c, temp, MAX_FILENAME + MAX_PATH_LEN, 0);
        if (bytes_recv < 0)
        {
            perror("recv");
            exit(1);
        }
        if (bytes_recv == 0)
        {
            return;
        }
    }
    strcpy(duplicate_signal, "ok");
    if (send(c, duplicate_signal, 20, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    strcpy(full_path, temp);
    if (mkdir(full_path, 0755) == 0)
    {  // Use 0755 for standard permissions
        printf("Successfully created folder: %s\n", full_path);
        char log_details[512];
        snprintf(log_details, sizeof(log_details), "Folder created: %s", full_path);
        writeLog("ADD_FOLDER", "SUCCESS", log_details);
    }
    else
    {
        perror("mkdir");
        char log_details[512];
        snprintf(log_details, sizeof(log_details), "Failed to create folder: %s", full_path);
        writeLog("ADD_FOLDER", "ERROR", log_details);
    }
}

void deleteFolder(int c)
{
    char full_path[MAX_FILENAME + MAX_PATH_LEN];
    int bytes_recv = recv(c, full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        return;  // Don't exit the whole server
    }
    full_path[bytes_recv] = '\0';
    char command[MAX_FILENAME + MAX_PATH_LEN + 100];
    sprintf(command, "rm -rf \"%s\"", full_path);
    if (system(command) == 0)
    {
        printf("Successfully deleted folder: %s\n", full_path);
        char log_details[512];
        snprintf(log_details, sizeof(log_details), "Folder deleted: %s", full_path);
        writeLog("DELETE_FOLDER", "SUCCESS", log_details);
    }
    else
    {
        perror("system");
        char log_details[512];
        snprintf(log_details, sizeof(log_details), "Failed to delete folder: %s", full_path);
        writeLog("DELETE_FOLDER", "ERROR", log_details);
    }
}

void renameFileOrFolder(int c)
{
    char old_full_path[MAX_FILENAME + MAX_PATH_LEN];
    char new_full_path[MAX_FILENAME + MAX_PATH_LEN];
    int bytes_recv;

    // Receive old name
    bytes_recv = recv(c, old_full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv old_name");
        return;
    }
    old_full_path[bytes_recv] = '\0';
    printf("Received old name: %s\n", old_full_path);

    // Receive new name
    bytes_recv = recv(c, new_full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv <= 0)
    {
        perror("recv new_name");
        return;
    }
    printf("Received new name: %s\n", new_full_path);
    new_full_path[bytes_recv] = '\0';

    if (rename(old_full_path, new_full_path) == 0)
    {
        printf("Successfully renamed '%s' to '%s'\n", old_full_path, new_full_path);
        char log_details[1024];
        snprintf(log_details, sizeof(log_details), "Renamed '%s' to '%s'", old_full_path,
                 new_full_path);
        writeLog("RENAME", "SUCCESS", log_details);
    }
    else
    {
        perror("rename");
        char log_details[1024];
        snprintf(log_details, sizeof(log_details), "Failed to rename '%s' to '%s'", old_full_path,
                 new_full_path);
        writeLog("RENAME", "ERROR", log_details);
    }
}

void download(int c)
{
    char full_path[MAX_FILENAME + MAX_PATH_LEN];
    char filename[MAX_FILENAME];
    int bytes_recv = recv(c, full_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    full_path[bytes_recv] = '\0';
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
    printf("Successfully downloaded file: %s\n", full_path);
    char log_details[512];
    snprintf(log_details, sizeof(log_details), "File downloaded: %s", full_path);
    writeLog("DOWNLOAD", "SUCCESS", log_details);
}

void paste(int c)
{
    char currentpath[MAX_PATH_LEN + MAX_FILENAME];
    int bytes_recv = recv(c, currentpath, MAX_PATH_LEN + MAX_FILENAME, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    currentpath[bytes_recv] = '\0';
    char copied_path[MAX_FILENAME + MAX_PATH_LEN];
    bytes_recv = recv(c, copied_path, MAX_FILENAME + MAX_PATH_LEN, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    copied_path[bytes_recv] = '\0';
    char temp[MAX_FILENAME + MAX_PATH_LEN];
    strcpy(temp, currentpath);
    printf("%s\n%s\n", copied_path, temp);
    char duplicate_signal[20];
    while (access(temp, F_OK) == 0)
    {
        strcpy(duplicate_signal, "duplicate");
        if (send(c, duplicate_signal, 20, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        bytes_recv = recv(c, temp, MAX_FILENAME + MAX_PATH_LEN, 0);
        if (bytes_recv < 0)
        {
            perror("recv");
            exit(1);
        }
        if (bytes_recv == 0)
        {
            return;
        }
    }
    strcpy(duplicate_signal, "ok");
    if (send(c, duplicate_signal, 20, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    strcpy(currentpath, temp);
    char command[MAX_FILENAME + MAX_PATH_LEN + 100];
    sprintf(command, "cp -r \"%s\" \"%s\"", copied_path, currentpath);
    if (system(command) == 0)
    {
        printf("Successfully pasted to: %s\n", currentpath);
        char log_details[1024];
        snprintf(log_details, sizeof(log_details), "Pasted '%s' to '%s'", copied_path, currentpath);
        writeLog("PASTE", "SUCCESS", log_details);
    }
    else
    {
        perror("system");
        char log_details[1024];
        snprintf(log_details, sizeof(log_details), "Failed to paste '%s' to '%s'", copied_path,
                 currentpath);
        writeLog("PASTE", "ERROR", log_details);
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
    else if (strcmp(command, "download") == 0)
    {
        download(c);
    }
    else if (strcmp(command, "paste") == 0)
    {
        paste(c);
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

    printf("Server2 listening on port 8080...\n");

    while (1)
    {
        socklen_t clen = sizeof(struct sockaddr);
        c = accept(s, (struct sockaddr *)&client_addr, &clen);
        if (c < 0)
        {
            perror("accept");
            continue;
        }

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

        pthread_detach(thread_id);
    }
}
