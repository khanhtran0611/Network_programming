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

struct sockaddr_in server_addr; // client_addr and c are not used in client.c

// void openGroupFolder()[

// ]

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

        if (send(s, "addFile", 1024, 0) < 0)
        {
            perror("send");
            exit(1);
        }
        if (send(s, my_basename, 1024, 0) < 0)
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
    GtkListStore *file_list_store = GTK_LIST_STORE(user_data);
    GtkWidget *dialog;
    // Use GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER to select folders
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    gint res;

    // Get the main window to be the parent of the dialog
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));

    dialog = gtk_file_chooser_dialog_new("Select Folder",
                                         GTK_WINDOW(toplevel),
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Select",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *folderpath;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        folderpath = gtk_file_chooser_get_filename(chooser);

        gchar *basename = g_path_get_basename(folderpath);
        g_print("Folder selected: %s\n", basename);

        GtkTreeIter iter;
        gtk_list_store_append(file_list_store, &iter);
        gtk_list_store_set(file_list_store, &iter, 0, basename, 1, "Folder", -1);

        g_free(basename);
        g_free(folderpath);
    }

    gtk_widget_destroy(dialog);
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

// Callback function when "Delete" is selected from the context menu
void on_menu_delete_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    int signal = 0;
    GtkTreePath *path = (GtkTreePath *)user_data;
    // The model is stored as a property on the menu item's parent menu
    GtkWidget *menu = gtk_widget_get_parent(GTK_WIDGET(menuitem));
    GtkTreeModel *model = (GtkTreeModel *)g_object_get_data(G_OBJECT(menu), "target-model");
    gchar *filename;
    char my_basename[1024];
    if (model && path)
    {
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, path))
        {
            gtk_tree_model_get(model, &iter, 0, &filename, -1);
            g_print("Context menu: 'Delete' clicked for item: %s\n", filename);

            // Remove the row from the list store
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            printf("%s\n", filename);

            signal = 1;
            // g_free(filename);
        }
    }
    snprintf(my_basename, sizeof(my_basename), "%s", filename);
    if (signal == 1)
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
        if (send(s, "deleteFile", 1024, 0) < 0)
        {
            printf("3\n");
            perror("send");
            exit(1);
        }

        if (send(s, my_basename, 1024, 0) < 0)
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

            GtkWidget *item_download = gtk_menu_item_new_with_label("Download");
            GtkWidget *item_delete = gtk_menu_item_new_with_label("Delete");

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_download);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_delete);

            // Get filename to pass to download callback
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
            GtkTreeIter iter;
            gtk_tree_model_get_iter(model, &iter, path);
            gchar *filename;
            gtk_tree_model_get(model, &iter, 0, &filename, -1);

            // Connect signals for menu items
            g_signal_connect(item_download, "activate", G_CALLBACK(on_menu_download_activate), g_strdup(filename));
            g_signal_connect(item_delete, "activate", G_CALLBACK(on_menu_delete_activate), gtk_tree_path_copy(path));
            g_object_set_data(G_OBJECT(menu), "target-model", model); // Store model for delete callback

            gtk_widget_show_all(menu);
            gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

            g_free(filename);
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

void get_list_of_files(GtkListStore *file_list_store) // Removed iter parameter
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
    if (send(s, command, strlen(command) + 1, 0) < 0)
    {
        perror("send command");
        exit(1);
    }

    uint32_t net_len;
    ssize_t bytes_read;

    while (1) // Loop indefinitely until a zero-length is received
    {
        // Receive filename length
        bytes_read = recv_all(s, &net_len, sizeof(net_len), 0);
        if (bytes_read <= 0)
        { // Error or connection closed unexpectedly
            if (bytes_read == 0)
            {
                printf("Server closed connection unexpectedly or sent no more data.\n");
            }
            else
            {
                perror("recv filename length");
            }
            break; // Exit loop
        }
        uint32_t name_len = ntohl(net_len);

        if (name_len == 0)
        { // Server signaled end of list
            printf("Received zero length, end of file list.\n");
            break; // Exit loop
        }

        // Allocate buffer for filename
        char *filename = (char *)malloc(name_len);
        if (filename == NULL)
        {
            perror("malloc filename");
            exit(1);
        }

        // Receive filename
        bytes_read = recv_all(s, filename, name_len, 0);
        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Server closed connection unexpectedly after name_len.\n");
            }
            else
            {
                perror("recv filename");
            }
            free(filename);
            break;
        }
        // filename is already null-terminated by server if name_len includes +1

        printf("Client received filename: %s\n", filename);

        // Receive type length
        bytes_read = recv_all(s, &net_len, sizeof(net_len), 0);
        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Server closed connection unexpectedly after filename.\n");
            }
            else
            {
                perror("recv type length");
            }
            free(filename);
            break;
        }
        uint32_t type_len = ntohl(net_len);

        // Allocate buffer for type
        char *type = (char *)malloc(type_len);
        if (type == NULL)
        {
            perror("malloc type");
            free(filename);
            exit(1);
        }

        // Receive type
        bytes_read = recv_all(s, type, type_len, 0);
        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Server closed connection unexpectedly after type_len.\n");
            }
            else
            {
                perror("recv type");
            }
            free(filename);
            free(type);
            break;
        }
        // type is already null-terminated by server if type_len includes +1

        printf("Client received type: %s\n", type);

        insert_list_tree(filename, type, file_list_store); // Call without iter

        free(filename);
        free(type);
    }
    close(s); // Close the local socket
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GtkWidget *window, *group_name_label, *members_listbox, *upload_button, *add_folder_button, *files_treeview;
    GtkListStore *file_list_store; // Thay đổi từ GtkTreeStore sang GtkListStore
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

    // --- Thêm dữ liệu dạng danh sách phẳng ---

    // Thêm folder "Source Code"
    // gtk_list_store_append(file_list_store, &iter);
    // gtk_list_store_set(file_list_store, &iter, 0, "Source Code", 1, "Folder", -1);

    // // Thêm file "ui_test.c"
    // gtk_list_store_append(file_list_store, &iter);
    // gtk_list_store_set(file_list_store, &iter, 0, "ui_test.c", 1, "File", -1);

    // // Thêm folder "Documents"
    // gtk_list_store_append(file_list_store, &iter);
    // gtk_list_store_set(file_list_store, &iter, 0, "Documents", 1, "Folder", -1);

    // // Thêm file "report.docx"
    // gtk_list_store_append(file_list_store, &iter);
    // gtk_list_store_set(file_list_store, &iter, 0, "report.docx", 1, "File", -1);

    // --- Kết thúc thêm dữ liệu ---

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

    get_list_of_files(file_list_store); // Call without iter

    // 4. Hiển thị cửa sổ và bắt đầu vòng lặp chính của ứng dụng
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
// {
//     perror("recv");
//     exit(1);
// }
// filename[bytes_recv] = '\0';
// printf("%s\n", filename);
// // if (strcmp(filename, "FIN!") == 0)
// // {
// //     break;
// // }
// bytes_recv = recv(s, type, 10, 0);
// type[bytes_recv] = '\0';
// printf("%s\n", type);
// insert_list_tree(filename, type, file_list_store, iter);
// }
// close(s);
// }

// int main(int argc, char *argv[])
// {
//     GtkBuilder *builder;
//     GtkWidget *window, *group_name_label, *members_listbox, *upload_button, *add_folder_button;
//     GtkListStore *file_list_store; // Thay đổi từ GtkTreeStore sang GtkListStore
//     GtkTreeIter iter;
//     GError *error = NULL;

//     // 1. Khởi tạo môi trường GTK+
//     gtk_init(&argc, &argv);

//     // 2. Tạo GtkBuilder và tải tệp .glade
//     builder = gtk_builder_new();
//     if (gtk_builder_add_from_file(builder, "group_members_screen.glade", &error) == 0)
//     {
//         // Xử lý lỗi nếu không tải được tệp
//         g_printerr("Lỗi khi tải tệp UI: %s\n", error->message);
//         g_clear_error(&error);
//         return 1;
//     }

//     // 3. Lấy widget cửa sổ chính (sử dụng ID bạn đặt trong Glade)
//     window = GTK_WIDGET(gtk_builder_get_object(builder, "group_members_window"));

//     // --- Bắt đầu thêm dữ liệu vào UI ---

//     // 3.1. Lấy con trỏ tới các widget khác bằng ID
//     group_name_label = GTK_WIDGET(gtk_builder_get_object(builder, "group_name_label"));
//     members_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "members_listbox"));
//     file_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "file_list_store")); // Ép kiểu sang GtkListStore
//     upload_button = GTK_WIDGET(gtk_builder_get_object(builder, "upload_file_button"));    // Giả sử ID của nút là "upload_file_button"
//     add_folder_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_folder_button"));

//     // 3.2. Thay đổi tên nhóm
//     gtk_label_set_text(GTK_LABEL(group_name_label), "Members of 'C Project Group'");

//     // 3.3. Xóa dữ liệu mẫu và thêm thành viên mới vào GtkListBox
//     // Lấy tất cả các con của listbox và xóa chúng
//     GList *children, *iter_list;
//     children = gtk_container_get_children(GTK_CONTAINER(members_listbox));
//     for (iter_list = children; iter_list != NULL; iter_list = g_list_next(iter_list))
//     {
//         gtk_widget_destroy(GTK_WIDGET(iter_list->data));
//     }
//     g_list_free(children);

//     // Thêm các thành viên mới
//     const gchar *members[] = {"alice@example.com", "bob@example.com", "charlie@example.com", NULL};
//     for (int i = 0; members[i] != NULL; i++)
//     {
//         GtkWidget *new_row = create_member_row(members[i]);
//         gtk_container_add(GTK_CONTAINER(members_listbox), new_row);

//         // SỬA LỖI: Lấy button từ GtkBox một cách chính xác
//         // GtkListBoxRow là một GtkBin, con của nó là GtkBox.
//         GtkWidget *box_in_row = gtk_bin_get_child(GTK_BIN(new_row));
//         // GtkBox chứa nhiều con. Lấy danh sách các con và lấy widget cuối cùng (là button).
//         GList *box_children = gtk_container_get_children(GTK_CONTAINER(box_in_row));
//         GtkWidget *button = g_list_last(box_children)->data;
//         g_list_free(box_children); // Giải phóng danh sách sau khi dùng

//         g_signal_connect(button, "clicked", G_CALLBACK(on_remove_clicked), NULL);
//     }

//     // 3.4. Xóa dữ liệu mẫu và thêm file/folder mới vào GtkListStore
//     gtk_list_store_clear(file_list_store);

//     // --- Thêm dữ liệu dạng danh sách phẳng ---

//     // Thêm folder "Source Code"
//     // gtk_list_store_append(file_list_store, &iter);
//     // gtk_list_store_set(file_list_store, &iter, 0, "Source Code", 1, "Folder", -1);

//     // // Thêm file "ui_test.c"
//     // gtk_list_store_append(file_list_store, &iter);
//     // gtk_list_store_set(file_list_store, &iter, 0, "ui_test.c", 1, "File", -1);

//     // // Thêm folder "Documents"
//     // gtk_list_store_append(file_list_store, &iter);
//     // gtk_list_store_set(file_list_store, &iter, 0, "Documents", 1, "Folder", -1);

//     // // Thêm file "report.docx"
//     // gtk_list_store_append(file_list_store, &iter);
//     // gtk_list_store_set(file_list_store, &iter, 0, "report.docx", 1, "File", -1);

//     // --- Kết thúc thêm dữ liệu ---

//     // Kết nối sự kiện cho nút Upload File
//     g_signal_connect(upload_button, "clicked", G_CALLBACK(on_upload_file_clicked), file_list_store);
//     g_signal_connect(add_folder_button, "clicked", G_CALLBACK(on_add_folder_clicked), file_list_store);

//     // Đặt hàm xử lý sự kiện đóng cửa sổ
//     g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(8080);
//     server_addr.sin_addr.s_addr = inet_addr("172.29.207.94");

//     get_list_of_files(file_list_store, iter);

//     // 4. Hiển thị cửa sổ và bắt đầu vòng lặp chính của ứng dụng
//     gtk_widget_show_all(window);
//     gtk_main();

//     return 0;
// }