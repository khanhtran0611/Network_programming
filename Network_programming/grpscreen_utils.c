#include "grpscreen_utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>  // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util2.h"

char BASE_PATH[MAX_PATH_LEN] = "Group_folders/";
char *root_path = "Group_folders/";
char copied_path[MAX_PATH_LEN + MAX_FILENAME];
char copied_type[10];

void init_grp_screen(GtkBuilder *builder){
    grp_screen = GTK_WIDGET(gtk_builder_get_object(builder, "group_members_window"));
    group_name_label = GTK_WIDGET(gtk_builder_get_object(builder, "group_name_label"));
    members_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "members_listbox"));
    file_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "file_list_store"));
    upload_button = GTK_WIDGET(gtk_builder_get_object(builder, "upload_file_button"));
    add_folder_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_folder_button"));
    files_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "files_treeview"));
    folder_back_button = GTK_WIDGET(gtk_builder_get_object(builder, "folder_back_button"));
    gtk_list_store_clear(file_list_store);
    g_signal_connect(upload_button, "clicked", G_CALLBACK(on_upload_file_clicked), file_list_store);
    g_signal_connect(add_folder_button, "clicked", G_CALLBACK(on_add_folder_clicked),
                     file_list_store);
    g_signal_connect(folder_back_button, "clicked", G_CALLBACK(on_folder_back_button_clicked),
                     NULL);
    g_signal_connect(files_treeview, "button-press-event",
                     G_CALLBACK(on_files_treeview_button_press_grp), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("172.29.207.94");
}

char *show_rename_dialog(char *old_filename)
{
    GtkWidget *dialog, *content_area, *entry, *label;
    char *new_filename = NULL;

    dialog = gtk_dialog_new_with_buttons(
        "Duplicate File", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Rename",
        GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);
    gtk_box_set_spacing(GTK_BOX(content_area), 5);

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "File '%s' already exists. Please enter a new name:", old_filename);
    label = gtk_label_new(buffer);
    gtk_container_add(GTK_CONTAINER(content_area), label);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), old_filename);
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && *text)
        {
            new_filename = g_strdup(text);
        }
    }
    gtk_widget_destroy(dialog);
    return new_filename;
}

char *check_name_duplicate(int s, char *current_path)
{
    int bytes_recv;
    char duplicate_signal[20];

    bytes_recv = recv(s, duplicate_signal, 20, 0);
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    char *new_filename;
    char temp[MAX_FILENAME + MAX_PATH_LEN];
    strcpy(temp, current_path);
    while (strcmp(duplicate_signal, "duplicate") == 0)
    {
        new_filename = show_rename_dialog(basename(current_path));
        if (new_filename == NULL)
        {
            return NULL;
        }
        strcpy(temp, BASE_PATH);
        strcat(temp, new_filename);
        // strcpy(current_path, temp);
        g_free(new_filename);
        if (send(s, temp, MAX_FILENAME + MAX_PATH_LEN, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        bytes_recv = recv(s, duplicate_signal, 20, 0);
        if (bytes_recv < 0)
        {
            perror("recv");
            exit(1);
        }
    }
    strcpy(current_path, temp);
    return current_path;
}

void insert_list_tree(char *filename, char *type,
                      GtkListStore *file_list_store)  // Removed iter parameter

{
    GtkTreeIter iter;  // Declare iter locally
    gtk_list_store_append(file_list_store, &iter);
    gtk_list_store_set(file_list_store, &iter, 0, filename, 1, type, -1);
}

void on_remove_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *row = gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(button)));
    GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(row));
    g_print("Removing member...\n");
    gtk_container_remove(GTK_CONTAINER(list_box), row);
}

void on_upload_file_clicked(GtkButton *button, gpointer user_data)
{
    int signal = 0;
    GtkListStore *file_list_store = GTK_LIST_STORE(user_data);
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    // Lấy cửa sổ chính để làm cha cho dialog
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));

    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(toplevel), action, "_Cancel",
                                         GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    char *filepath;
    char *my_basename;
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filepath = gtk_file_chooser_get_filename(chooser);

        gchar *basename = g_path_get_basename(filepath);

        my_basename = basename;

        // In các chuỗi gchar* ra console để kiểm tra
        g_print("File path (đường dẫn đầy đủ): %s\n", filepath);
        g_print("Base name (tên file): %s\n", basename);
        // Sử dụng biến char* mới với hàm printf chuẩn của C
        printf("Using my_basename (char *): %s\n", my_basename);
        // ----- KẾT THÚC PHẦN CHỈNH SỬA -----

        // Thêm file vào GtkListStore

        // Giải phóng bộ nhớ sau khi sử dụng
        // g_free(basename);
        // g_free(filepath);
        signal = 1;
    }

    gtk_widget_destroy(dialog);
    if (signal == 1)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // Use local s
        if (s == -1)
        {
            perror("socket");
            exit(1);
        }
        if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("connect");
            exit(1);
        }
        char full_path[MAX_PATH_LEN + MAX_FILENAME];
        strcpy(full_path, BASE_PATH);
        strcat(full_path, my_basename);

        if (send(s, "addFile", COMMAND_LENGTH, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        if (send(s, full_path, MAX_PATH_LEN + MAX_FILENAME, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        char *new_path = check_name_duplicate(s, full_path);
        if (new_path)
        {
            GtkTreeIter iter;
            gtk_list_store_append(file_list_store, &iter);
            gtk_list_store_set(file_list_store, &iter, 0, basename(new_path), 1, "file", -1);
        }
        else
        {
            close(s);
            g_free(my_basename);
            g_free(filepath);
            return;
        }
        FILE *fp = fopen(filepath, "rb");
        if (fp == NULL)
        {
            perror("fopen");
            exit(1);
        }
        char buffer[10240];
        int bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            if (send(s, buffer, bytes_read, 0) < 0)
            {
                perror("send");
                exit(1);
            }
        }
        fclose(fp);
        // Thêm dòng này: Báo cho server biết đã gửi xong file.
        // shutdown(s, SHUT_WR) sẽ gửi một gói tin FIN đến server,
        // làm cho recv() của server trả về 0.
        if (shutdown(s, SHUT_WR) < 0)
        {
            perror("shutdown");
        }

        close(s);

        g_free(my_basename);
        g_free(filepath);
    }
}

void on_add_folder_clicked(GtkButton *button, gpointer user_data)
{
    int signal = 0;
    GtkListStore *file_list_store = GTK_LIST_STORE(user_data);
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;
    gchar *foldername = NULL;

    // Lấy cửa sổ chính để làm cha cho dialog
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));

    // Tạo dialog mới với các nút "Create" và "Cancel"
    dialog =
        gtk_dialog_new_with_buttons("Create New Folder", GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Create",
                                    GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    // Lấy vùng nội dung của dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Tạo một GtkEntry để người dùng nhập tên thư mục
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter folder name...");
    gtk_widget_set_margin_start(entry, 10);
    gtk_widget_set_margin_end(entry, 10);
    gtk_widget_set_margin_top(entry, 10);
    gtk_widget_set_margin_bottom(entry, 10);
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show(entry);

    gint res;
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && *text)  // Check if the text is not null and not empty
        {
            foldername = g_strdup(text);  // Make a copy
            g_print("Folder to create: %s\n", foldername);
            signal = 1;
            // Thêm vào giao diện trước để có cảm giác phản hồi nhanh
        }
    }
    char *my_folder_name = foldername;
    gtk_widget_destroy(dialog);

    if (signal == 1)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // Use local s
        if (s == -1)
        {
            perror("socket");
            exit(1);
        }
        if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("connect");
            exit(1);
        }
        if (send(s, "addFolder", COMMAND_LENGTH, 0) < 0)
        {
            perror("send");
            exit(1);
        }

        char folder_path[MAX_FILENAME + MAX_PATH_LEN];
        strcpy(folder_path, BASE_PATH);
        strcat(folder_path, my_folder_name);

        if (send(s, folder_path, MAX_FILENAME + MAX_PATH_LEN, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        char *new_folder_path = check_name_duplicate(s, folder_path);
        if (new_folder_path)
        {
            GtkTreeIter iter;
            gtk_list_store_append(file_list_store, &iter);
            gtk_list_store_set(file_list_store, &iter, 0, basename(new_folder_path), 1, "folder",
                               -1);
        }
        close(s);
        signal = 0;
    }
    g_free(foldername);
}

void on_folder_back_button_clicked(GtkButton *button, gpointer user_data)
{
    if (file_list_store)
    {
        gtk_list_store_clear(file_list_store);
    }
    else
    {
        g_printerr("file_list_store is NULL. Cannot clear.\n");
        return;
    }
    dirname(BASE_PATH);
    if (strcmp(BASE_PATH, "/") == 0)
    {
        return;
    }
    strcat(BASE_PATH, "/");
    get_list_of_files(file_list_store);
}

void on_menu_copy_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GtkTreePath *path = (GtkTreePath *)user_data;
    GtkWidget *menu = gtk_widget_get_parent(GTK_WIDGET(menuitem));
    GtkTreeModel *model = (GtkTreeModel *)g_object_get_data(G_OBJECT(menu), "target-model");
    GtkTreeIter iter;
    gchar *filename = NULL;
    gchar *type = NULL;

    if (model && path && gtk_tree_model_get_iter(model, &iter, path))
    {
        // Get the filename from the first column (index 0)
        gtk_tree_model_get(model, &iter, 0, &filename, 1, &type, -1);

        if (filename)
        {
            // Get the system clipboard
            GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            // Set the clipboard text
            gtk_clipboard_set_text(clipboard, filename, -1);
            g_print("Copied '%s' to clipboard.\n", filename);
            strcpy(copied_path, BASE_PATH);
            strcat(copied_path, filename);
            if (type)
            {
                strcpy(copied_type, type);
                g_free(type);
            }
            else
            {
                strcpy(copied_type, "File");
            }
            g_free(filename);
        }
    }

    gtk_tree_path_free(path);
}

void on_menu_download_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    char *filename = (char *)user_data;
    g_print("Context menu: 'Download' clicked for file: %s\n", filename);
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // Use local s
    if (s == -1)
    {
        perror("socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    if (send(s, "download", COMMAND_LENGTH, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    char full_path[MAX_FILENAME + MAX_PATH_LEN];
    strcpy(full_path, BASE_PATH);
    strcat(full_path, filename);

    if (send(s, full_path, MAX_FILENAME + MAX_PATH_LEN, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    char buffer[10240];
    int bytes_recv;
    char filepath[MAX_FILENAME + MAX_PATH_LEN];
    strcpy(filepath, download_path);
    strcat(filepath, "/");
    strcat(filepath, filename);
    printf("%s\n", filepath);
    FILE *fp = fopen(filepath, "wb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(1);
    }
    while ((bytes_recv = recv(s, buffer, 10240, 0)) > 0)
    {
        printf("Wrote %d bytes \n", bytes_recv);
        fwrite(buffer, 1, bytes_recv, fp);
    }
    fclose(fp);  // Di chuyển fclose() lên trước khi kiểm tra lỗi
    if (bytes_recv < 0)
    {
        perror("recv");
        exit(1);
    }
    g_free(filename);  // Free the duplicated filename string
}

void on_menu_rename_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    GtkTreePath *path = (GtkTreePath *)user_data;
    GtkWidget *menu = gtk_widget_get_parent(GTK_WIDGET(menuitem));
    GtkTreeModel *model = (GtkTreeModel *)g_object_get_data(G_OBJECT(menu), "target-model");
    GtkTreeIter iter;
    gchar *old_filename = NULL;
    gchar *new_filename;
    if (model && path && gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_tree_model_get(model, &iter, 0, &old_filename, -1);

        // Create a dialog to get the new name
        GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(menu));
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Rename Item", GTK_WINDOW(toplevel), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            "_Rename", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), old_filename);
        gtk_container_add(GTK_CONTAINER(content_area), entry);
        gtk_widget_show(entry);

        int signal = 0;

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_ACCEPT)
        {
            const gchar *new_filename_const = gtk_entry_get_text(GTK_ENTRY(entry));
            if (new_filename_const && *new_filename_const &&
                g_strcmp0(old_filename, new_filename_const) != 0)
            {
                new_filename = g_strdup(new_filename_const);
                g_print("Renaming '%s' to '%s'\n", old_filename, new_filename);

                // Send rename command to server
            }
            signal = 1;
        }

        gtk_widget_destroy(dialog);
        if (signal == 1)
        {
            int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s == -1)
            {
                perror("socket");
                exit(1);
            }
            if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                perror("connect");
                exit(1);
            }

            if (send(s, "rename", COMMAND_LENGTH, 0) < 0)
            {
                perror("send");
                exit(1);
            }

            char old_full_path[MAX_FILENAME + MAX_PATH_LEN];
            char new_full_path[MAX_FILENAME + MAX_PATH_LEN];

            snprintf(old_full_path, sizeof(old_full_path), "%s%s", BASE_PATH, old_filename);
            snprintf(new_full_path, sizeof(new_full_path), "%s%s", BASE_PATH, new_filename);

            if (send(s, old_full_path, MAX_FILENAME + MAX_PATH_LEN, 0) < 0)
            {
                perror("send");
                exit(1);
            }
            if (send(s, new_full_path, MAX_FILENAME + MAX_PATH_LEN, 0) < 0)
            {
                perror("send");
                exit(1);
            }
            close(s);
            // Update the model
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, new_filename, -1);
        }
        g_free(new_filename);
        g_free(old_filename);
    }
    gtk_tree_path_free(path);
}

void on_menu_view_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    char *foldername = (char *)user_data;
    g_print("Context menu: 'View' clicked for folder: %s\n", foldername);
    if (file_list_store)
    {
        gtk_list_store_clear(file_list_store);
    }
    else
    {
        g_printerr("file_list_store is NULL. Cannot clear.\n");
        g_free(foldername);
        return;
    }
    strncat(BASE_PATH, foldername, sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    strncat(BASE_PATH, "/", sizeof(BASE_PATH) - strlen(BASE_PATH) - 1);
    get_list_of_files(file_list_store);
    g_free(foldername);
}

void on_menu_delete_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    int signal = 0;
    GtkTreePath *path = (GtkTreePath *)user_data;
    // The model is stored as a property on the menu item's parent menu
    GtkWidget *menu = gtk_widget_get_parent(GTK_WIDGET(menuitem));
    GtkTreeModel *model = (GtkTreeModel *)g_object_get_data(G_OBJECT(menu), "target-model");
    gchar *filename;
    gchar *type;
    char my_basename[1024];
    char *command_to_send = NULL;
    if (model && path)
    {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, path))
        {
            // Lấy cả tên (cột 0) và loại (cột 1)
            gtk_tree_model_get(model, &iter, 0, &filename, 1, &type, -1);
            g_print("Context menu: 'Delete' clicked for item: %s, type: %s\n", filename, type);

            // Remove the row from the list store
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            printf("%s\n", filename);

            signal = 1;
            // g_free(filename);

            // Xác định lệnh cần gửi dựa trên loại
            if (g_strcmp0(type, "folder") == 0)
            {
                command_to_send = "deleteFolder";
            }
            else
            {
                command_to_send = "deleteFile";
            }
            g_free(type);  // Giải phóng bộ nhớ cho 'type'
        }
    }
    snprintf(my_basename, sizeof(my_basename), "%s", filename);
    if (signal == 1 && command_to_send != NULL)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // Use local s
        if (s == -1)
        {
            printf("1\n");
            perror("socket");
            exit(1);
        }
        if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            printf("2\n");
            perror("connect");
            exit(1);
        }
        if (send(s, command_to_send, COMMAND_LENGTH, 0) < 0)
        {
            printf("3\n");
            perror("send");
            exit(1);
        }

        char full_path[MAX_FILENAME + MAX_PATH_LEN];
        strcpy(full_path, BASE_PATH);
        strcat(full_path, my_basename);

        if (send(s, full_path, MAX_PATH_LEN + MAX_FILENAME, 0) < 0)
        {
            printf("4\n");
            perror("send");
            exit(1);
        }
        close(s);
        signal = 0;
    }
    g_free(filename);
    gtk_tree_path_free(path);  // Free the path
}

void on_menu_paste_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    // Logic for pasting a file/folder will be added here.
    // For now, it just prints a message.
    g_print("Paste action triggered.\n");
    char filename[MAX_FILENAME];
    strcpy(filename, basename(copied_path));
    // strcat(copied_path, "/");
    // strcat(copied_path, filename);
    char current_path[MAX_PATH_LEN + MAX_FILENAME];
    strcpy(current_path, BASE_PATH);
    strcat(current_path, filename);

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        perror("socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    if (send(s, "paste", COMMAND_LENGTH, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    // Send current path
    if (send(s, current_path, MAX_PATH_LEN + MAX_FILENAME, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    // Send copied path
    if (send(s, copied_path, MAX_PATH_LEN + MAX_FILENAME, 0) < 0)
    {
        perror("send");
        exit(1);
    }
    char *new_path = check_name_duplicate(s, current_path);
    if (!new_path)
    {
        close(s);
        return;
    }
    close(s);

    // gchar *final_basename = g_path_get_basename(current_path);
    GtkTreeIter iter;
    gtk_list_store_append(file_list_store, &iter);
    gtk_list_store_set(file_list_store, &iter, 0, basename(new_path), 1, copied_type, -1);

    // Example: You might get data from the clipboard like this:
    // GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    // gtk_clipboard_request_text(clipboard, on_paste_text_received, user_data);
}

gboolean on_files_treeview_button_press_grp(GtkWidget *treeview, GdkEventButton *event,
                                            gpointer user_data)
{
    // Check for a right-click (button 3)
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkTreePath *path;
        gboolean on_item = gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint)event->x,
                                                         (gint)event->y, &path, NULL, NULL, NULL);

        if (on_item)
        {
            // --- START: Click was on an item, show the existing menu ---
            GtkWidget *menu = gtk_menu_new();

            GtkWidget *item_view = gtk_menu_item_new_with_label("View");
            GtkWidget *item_download = gtk_menu_item_new_with_label("Download");
            GtkWidget *item_copy = gtk_menu_item_new_with_label("Copy");
            GtkWidget *item_rename = gtk_menu_item_new_with_label("Rename");
            GtkWidget *item_delete = gtk_menu_item_new_with_label("Delete");

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_view);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_download);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_copy);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_rename);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_delete);

            // Get filename and type to decide which items to enable
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
            GtkTreeIter iter;
            gtk_tree_model_get_iter(model, &iter, path);
            gchar *filename;
            gchar *type;
            gtk_tree_model_get(model, &iter, 0, &filename, 1, &type, -1);

            // "View" is only for folders
            if (g_strcmp0(type, "folder") != 0)
            {
                gtk_widget_set_sensitive(item_view, FALSE);
            }

            // "Download" is only for files
            if (g_strcmp0(type, "folder") == 0)
            {
                gtk_widget_set_sensitive(item_download, FALSE);
            }

            // Connect signals for menu items
            g_signal_connect(item_view, "activate", G_CALLBACK(on_menu_view_activate),
                             g_strdup(filename));
            g_signal_connect(item_download, "activate", G_CALLBACK(on_menu_download_activate),
                             g_strdup(filename));
            g_signal_connect(item_copy, "activate", G_CALLBACK(on_menu_copy_activate),
                             gtk_tree_path_copy(path));
            g_signal_connect(item_rename, "activate", G_CALLBACK(on_menu_rename_activate),
                             gtk_tree_path_copy(path));
            g_signal_connect(item_delete, "activate", G_CALLBACK(on_menu_delete_activate),
                             gtk_tree_path_copy(path));
            g_object_set_data(G_OBJECT(menu), "target-model",
                              model);  // Store model for delete callback

            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

            g_free(filename);
            g_free(type);
            gtk_tree_path_free(path);

            return TRUE;  // Event handled, stop propagation
            // --- END: Click was on an item ---
        }
        else
        {
            // --- START: Click was on an empty area, show "Paste" menu ---
            GtkWidget *menu = gtk_menu_new();
            GtkWidget *item_paste = gtk_menu_item_new_with_label("Paste");

            // You might want to check if the clipboard is empty
            // and disable "Paste" if it is. For now, it's always enabled.
            // GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            // if (!gtk_clipboard_wait_is_text_available(clipboard)) {
            //     gtk_widget_set_sensitive(item_paste, FALSE);
            // }

            g_signal_connect(item_paste, "activate", G_CALLBACK(on_menu_paste_activate), user_data);

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_paste);

            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

            return TRUE;  // Event handled, stop propagation
            // --- END: Click was on an empty area ---
        }
    }
    return FALSE;  // Event not handled, continue propagation
}

void on_window_destroy()
{
    gtk_main_quit();
}
