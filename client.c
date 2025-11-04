#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include <stdint.h> // For uint32_t

#define MAX_FILENAME 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 16

struct sockaddr_in server_addr; // client_addr and c are not used in client.c
GtkListStore *file_list_store;


// A helper function to create a new row for the members list
GtkWidget *create_member_row(const gchar *email)
{
    GtkWidget *row, *box, *label, *button;

    // Create a new row
    row = gtk_list_box_row_new();

    // Create a horizontal box to hold the label and button
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);

    // Create the email label
    label = gtk_label_new(email);
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    // Create the "Remove" button
    button = gtk_button_new_with_label("Remove");
    // Add the "destructive-action" style class to make it red, like in Glade
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "destructive-action");

    // Pack the label and button into the box
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(box), button, FALSE, TRUE, 0);

    // Add the box to the row
    gtk_container_add(GTK_CONTAINER(row), box);

    return row;
}

// Callback for the "Remove" button clicks (for demonstration)
void on_remove_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *row = gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(button)));
    GtkListBox *list_box = GTK_LIST_BOX(gtk_widget_get_parent(row));
    g_print("Removing member...\n");
    gtk_container_remove(GTK_CONTAINER(list_box), row);
}

// Callback for the "Upload File" button
void on_upload_file_clicked(GtkButton *button, gpointer user_data)
{
    int signal = 0;
    GtkListStore *file_list_store = GTK_LIST_STORE(user_data);
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    // Lấy cửa sổ chính để làm cha cho dialog
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));

    dialog = gtk_file_chooser_dialog_new("Open File",
                                         GTK_WINDOW(toplevel),
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

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
        GtkTreeIter iter;
        gtk_list_store_append(file_list_store, &iter);
        gtk_list_store_set(file_list_store, &iter, 0, basename, 1, "File", -1);

        // Giải phóng bộ nhớ sau khi sử dụng
        // g_free(basename);
        // g_free(filepath);
        signal = 1;
    }

    gtk_widget_destroy(dialog);
    if (signal == 1)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use local s
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

        if (send(s, "addFile", COMMAND_LENGTH, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        if (send(s, my_basename, MAX_FILENAME, 0) < 0)
        {
            perror("send");
            exit(1);
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

// Callback for the "Add Folder" button
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
    dialog = gtk_dialog_new_with_buttons("Create New Folder",
                                         GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         "_Create",
                                         GTK_RESPONSE_ACCEPT,
                                         "_Cancel",
                                         GTK_RESPONSE_REJECT,
                                         NULL);

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
        if (text && *text) // Check if the text is not null and not empty
        {
            foldername = g_strdup(text); // Make a copy
            g_print("Folder to create: %s\n", foldername);

            // Thêm vào giao diện trước để có cảm giác phản hồi nhanh
            GtkTreeIter iter;
            gtk_list_store_append(file_list_store, &iter);
            gtk_list_store_set(file_list_store, &iter, 0, foldername, 1, "Folder", -1);
            signal = 1;
        }
    }
    char* my_folder_name = foldername;
    gtk_widget_destroy(dialog);

    if (signal == 1)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use local s
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

        if (send(s, my_folder_name, MAX_FILENAME, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        close(s);
        signal = 0;
    }
    g_free(foldername);
}

// --- START: Right-click menu functions ---

// Callback function when "Download" is selected from the context menu
void on_menu_download_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    char *filename = (char *)user_data;
    g_print("Context menu: 'Download' clicked for file: %s\n", filename);

    // TODO: Implement the download logic here.
    // You will need to create a socket, connect to the server,
    // send a "download" command along with the filename,
    // and then receive the file data.

    g_free(filename); // Free the duplicated filename string
}

// Callback function when "Rename" is selected from the context menu
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
        GtkWidget *dialog = gtk_dialog_new_with_buttons("Rename Item",
                                                        GTK_WINDOW(toplevel),
                                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        "_Rename", GTK_RESPONSE_ACCEPT,
                                                        "_Cancel", GTK_RESPONSE_REJECT,
                                                        NULL);
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
            if (new_filename_const && *new_filename_const && g_strcmp0(old_filename, new_filename_const) != 0)
            {
                new_filename = g_strdup(new_filename_const);
                g_print("Renaming '%s' to '%s'\n", old_filename, new_filename);

                // Send rename command to server

            }
            signal = 1;
        }

        gtk_widget_destroy(dialog);
        if(signal == 1){
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
            
        if(send(s, "rename", COMMAND_LENGTH, 0) < 0) 
        {
            perror("send");
            exit(1);
        }
        if(send(s, old_filename, MAX_FILENAME, 0) < 0) 
        {
            perror("send");
            exit(1);
        }
        if(send(s, new_filename, MAX_FILENAME, 0) < 0) 
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

// Callback function when "View" is selected from the context menu
void on_menu_view_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    char *foldername = (char *)user_data;
    g_print("Context menu: 'View' clicked for folder: %s\n", foldername);
        if (file_list_store) {
        gtk_list_store_clear(file_list_store);
    } else {
        g_printerr("file_list_store is NULL. Cannot clear.\n");
        g_free(foldername);
        return;
    }
    //     int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use local s
    // if (s == -1)
    // {
    //     perror("socket");
    //     exit(1);
    // }
    // if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    // {
    //     perror("connect");
    //     exit(1);
    // }
    // char *command = "getList";
    // if (send(s, command, strlen(command) + 1, 0) < 0)
    // {
    //     perror("send command");
    //     exit(1);
    // }
    // get_list_of_files(file_list_store, command, s);
    
    
    // close(s);
    g_free(foldername);
}
// Callback function when "Delete" is selected from the context menu
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
            if (g_strcmp0(type, "folder") == 0) {
                command_to_send = "deleteFolder";
            } else {
                command_to_send = "deleteFile";
            }
            g_free(type); // Giải phóng bộ nhớ cho 'type'
        }
    }
    snprintf(my_basename, sizeof(my_basename), "%s", filename);
    if (signal == 1 && command_to_send != NULL)
    {
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use local s
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

        if (send(s, my_basename, MAX_FILENAME, 0) < 0)
        {
            printf("4\n");
            perror("send");
            exit(1);
        }
        close(s);
        signal = 0;
    }
    g_free(filename);
    gtk_tree_path_free(path); // Free the path
}

// Callback for button press events on the TreeView
gboolean on_files_treeview_button_press(GtkWidget *treeview, GdkEventButton *event, gpointer user_data)
{
    // Check for a right-click (button 3)
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkTreePath *path;
        // Get the path at the click coordinates. If a row is not clicked, do nothing.
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL))
        {
            // Create the context menu
            GtkWidget *menu = gtk_menu_new();

            GtkWidget *item_view = gtk_menu_item_new_with_label("View");
            GtkWidget *item_download = gtk_menu_item_new_with_label("Download");
            GtkWidget *item_rename = gtk_menu_item_new_with_label("Rename");
            GtkWidget *item_delete = gtk_menu_item_new_with_label("Delete");
            
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_view);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_download);
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
            if (g_strcmp0(type, "folder") != 0) {
                gtk_widget_set_sensitive(item_view, FALSE);
            }

            // Connect signals for menu items
            g_signal_connect(item_view, "activate", G_CALLBACK(on_menu_view_activate), g_strdup(filename));
            g_signal_connect(item_download, "activate", G_CALLBACK(on_menu_download_activate), g_strdup(filename));
            g_signal_connect(item_rename, "activate", G_CALLBACK(on_menu_rename_activate), gtk_tree_path_copy(path));
            g_signal_connect(item_delete, "activate", G_CALLBACK(on_menu_delete_activate), gtk_tree_path_copy(path));
            g_object_set_data(G_OBJECT(menu), "target-model", model); // Store model for delete callback

            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

            g_free(filename);
            g_free(type);
            gtk_tree_path_free(path);

            return TRUE; // Event handled, stop propagation
        }
    }
    return FALSE; // Event not handled, continue propagation
}

// --- END: Right-click menu functions ---

void insert_list_tree(char *filename, char *type, GtkListStore *file_list_store) // Removed iter parameter
{
    GtkTreeIter iter; // Declare iter locally
    gtk_list_store_append(file_list_store, &iter);
    gtk_list_store_set(file_list_store, &iter, 0, filename, 1, type, -1);
}

// Helper function to ensure all bytes are received
ssize_t recv_all(int sockfd, void *buf, size_t len, int flags)
{
    size_t total_received = 0;
    ssize_t bytes_left = len;
    ssize_t n;

    while (total_received < len)
    {
        n = recv(sockfd, (char *)buf + total_received, bytes_left, flags);
        if (n == -1)
        {
            return -1; // Error
        }
        if (n == 0)
        {
            // Peer closed connection. If we haven't received all expected bytes,
            // this is an unexpected closure.
            if (total_received < len)
            {
                errno = EPIPE; // Indicate broken pipe or unexpected EOF
                return -1;
            }
            return total_received; // Peer closed connection after sending all data
        }
        total_received += n;
        bytes_left -= n;
    }
    return total_received;
}

void get_list_of_files(GtkListStore *file_list_store, char* foldername) // Removed iter parameter
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use local s
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
    char *command = "getList";
    if (send(s, command, COMMAND_LENGTH, 0) < 0)
    {
        perror("send command");
        exit(1);
    }
    
    if(send(s,foldername,1024,0) < 0){
        perror("send");
        exit(1);
    }
    ssize_t bytes_read;

    while (1) // Loop indefinitely until a zero-length is received
    {
        // Allocate buffer for filename
        char filename[MAX_FILENAME];
        // Receive filename
        bytes_read = recv(s, filename, MAX_FILENAME, 0);
        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Server closed connection.\n");
            }
            else
            {
                perror("recv filename");
            }
            break;
        }
        // filename is already null-terminated by server if name_len includes +1

        printf("Client received filename: %s\n", filename);

        // Allocate buffer for type
        char type[10];
        if (type == NULL)
        {
            perror("malloc type");
            exit(1);
        }
        // Receive type
        bytes_read = recv(s, type, 10, 0);
        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Server closed connection unexpectedly after type_len.\n");
                break;
            }
            else
            {
                perror("recv type");
            }
            break;
        }
        // type is already null-terminated by server if type_len includes +1
        printf("Client received type: %s\n", type);
        insert_list_tree(filename, type, file_list_store); // Call without iter
    }
    close(s);  // Close the local socket
}

void viewOpeningFolder(GtkListStore *file_list_store){

    // get_list_of_files(file_list_store, command, s);

}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GtkWidget *window, *group_name_label, *members_listbox, *upload_button, *add_folder_button, *files_treeview;
 // Thay đổi từ GtkTreeStore sang GtkListStore
    // GtkTreeIter iter; // Removed iter declaration
    GError *error = NULL;

    // 1. Khởi tạo môi trường GTK+
    gtk_init(&argc, &argv);

    // 2. Tạo GtkBuilder và tải tệp .glade
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "group_members_screen.glade", &error) == 0)
    {
        // Xử lý lỗi nếu không tải được tệp
        g_printerr("Lỗi khi tải tệp UI: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    // 3. Lấy widget cửa sổ chính (sử dụng ID bạn đặt trong Glade)
    window = GTK_WIDGET(gtk_builder_get_object(builder, "group_members_window"));

    // --- Bắt đầu thêm dữ liệu vào UI ---

    // 3.1. Lấy con trỏ tới các widget khác bằng ID
    group_name_label = GTK_WIDGET(gtk_builder_get_object(builder, "group_name_label"));
    members_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "members_listbox"));
    file_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "file_list_store")); // Ép kiểu sang GtkListStore
    upload_button = GTK_WIDGET(gtk_builder_get_object(builder, "upload_file_button"));    // Giả sử ID của nút là "upload_file_button"
    add_folder_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_folder_button"));
    files_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "files_treeview"));

    // 3.2. Thay đổi tên nhóm
    gtk_label_set_text(GTK_LABEL(group_name_label), "Members of 'C Project Group'");

    // 3.3. Xóa dữ liệu mẫu và thêm thành viên mới vào GtkListBox
    // Lấy tất cả các con của listbox và xóa chúng
    GList *children, *iter_list;
    children = gtk_container_get_children(GTK_CONTAINER(members_listbox));
    for (iter_list = children; iter_list != NULL; iter_list = g_list_next(iter_list))
    {
        gtk_widget_destroy(GTK_WIDGET(iter_list->data));
    }
    g_list_free(children);

    // Thêm các thành viên mới
    const gchar *members[] = {"alice@example.com", "bob@example.com", "charlie@example.com", NULL};
    for (int i = 0; members[i] != NULL; i++)
    {
        GtkWidget *new_row = create_member_row(members[i]);
        gtk_container_add(GTK_CONTAINER(members_listbox), new_row);

        // SỬA LỖI: Lấy button từ GtkBox một cách chính xác
        // GtkListBoxRow là một GtkBin, con của nó là GtkBox.
        GtkWidget *box_in_row = gtk_bin_get_child(GTK_BIN(new_row));
        // GtkBox chứa nhiều con. Lấy danh sách các con và lấy widget cuối cùng (là button).
        GList *box_children = gtk_container_get_children(GTK_CONTAINER(box_in_row));
        GtkWidget *button = g_list_last(box_children)->data;
        g_list_free(box_children); // Giải phóng danh sách sau khi dùng

        g_signal_connect(button, "clicked", G_CALLBACK(on_remove_clicked), NULL);
    }

    // 3.4. Xóa dữ liệu mẫu và thêm file/folder mới vào GtkListStore
    gtk_list_store_clear(file_list_store);

    // Kết nối sự kiện cho nút Upload File
    g_signal_connect(upload_button, "clicked", G_CALLBACK(on_upload_file_clicked), file_list_store);
    g_signal_connect(add_folder_button, "clicked", G_CALLBACK(on_add_folder_clicked), file_list_store);

    // Kết nối sự kiện click chuột cho TreeView để hiển thị menu ngữ cảnh
    g_signal_connect(files_treeview, "button-press-event", G_CALLBACK(on_files_treeview_button_press), NULL);

    // Đặt hàm xử lý sự kiện đóng cửa sổ
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("172.29.207.94");

    get_list_of_files(file_list_store,"GroupA"); // Call without iter

    // 4. Hiển thị cửa sổ và bắt đầu vòng lặp chính của ứng dụng
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
