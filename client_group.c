#include <arpa/inet.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>  // For uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For memset
#include <sys/socket.h>
#include <unistd.h>
#include "database.h"

#define MAX_FILENAME 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 16

struct sockaddr_in server_addr;
GtkListStore *group_list_store;
GtkListStore *member_list_store;
GtkListStore *request_list_store;
char current_user_email[MAX_EMAIL_LEN] = "";
char current_username[MAX_USERNAME_LEN] = "";
int current_group_id = -1;
GtkWidget *main_window = NULL;
GtkWidget *user_label = NULL;
GtkWidget *login_button = NULL;
GtkWidget *logout_button = NULL;

// Forward declarations
void on_refresh_groups_clicked(GtkButton *button, gpointer user_data);
void on_leave_group_clicked(GtkButton *button, gpointer user_data);
void on_approve_request_clicked(GtkButton *button, gpointer user_data);
void load_members(GtkListBox *members_listbox);
void load_requests(GtkListBox *requests_listbox);
void show_login_dialog(void);
void show_login_dialog_from_button(GtkButton *button, gpointer user_data);
void show_register_dialog(void);
void show_register_dialog_from_button(GtkButton *button, gpointer user_data);
void on_logout_clicked(GtkButton *button, gpointer user_data);
void update_user_display(void);

// Helper function to send command and receive response
int send_command(const char *command, const char *data1, const char *data2, char *response,
                 size_t response_size)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        perror("socket");
        return -1;
    }

    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(s);
        return -1;
    }

    // Send command
    if (send(s, command, COMMAND_LENGTH, 0) < 0)
    {
        perror("send command");
        close(s);
        return -1;
    }

    // Send data if provided
    if (data1 != NULL)
    {
        size_t len1 = strlen(data1) + 1;  // Include null terminator
        if (send(s, data1, len1, 0) < 0)
        {
            perror("send data1");
            close(s);
            return -1;
        }
        // Pad to MAX_FILENAME if needed for compatibility
        if (len1 < MAX_FILENAME)
        {
            char padding[MAX_FILENAME];
            memset(padding, 0, MAX_FILENAME - len1);
            send(s, padding, MAX_FILENAME - len1, 0);
        }
    }

    if (data2 != NULL)
    {
        size_t len2 = strlen(data2) + 1;  // Include null terminator
        if (send(s, data2, len2, 0) < 0)
        {
            perror("send data2");
            close(s);
            return -1;
        }
        // Pad to MAX_FILENAME if needed for compatibility
        if (len2 < MAX_FILENAME)
        {
            char padding[MAX_FILENAME];
            memset(padding, 0, MAX_FILENAME - len2);
            send(s, padding, MAX_FILENAME - len2, 0);
        }
    }

    // Receive response
    ssize_t bytes_recv = recv(s, response, response_size - 1, 0);
    if (bytes_recv <= 0)
    {
        perror("recv response");
        close(s);
        return -1;
    }
    response[bytes_recv] = '\0';

    close(s);
    return 0;
}

// Wrapper for button callback
void show_login_dialog_from_button(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    show_login_dialog();
}

// Show login dialog
void show_login_dialog(void)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Login", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Login",
        GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    // Email entry
    GtkWidget *email_label = gtk_label_new("Email:");
    GtkWidget *email_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email_entry), "user@example.com");
    gtk_grid_attach(GTK_GRID(grid), email_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), email_entry, 1, 0, 1, 1);

    // Password entry
    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "password");
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);

    // Add Register button
    GtkWidget *register_button = gtk_dialog_add_button(GTK_DIALOG(dialog), "_Register", 100);
    (void)register_button;  // Suppress unused variable warning
    gtk_widget_show_all(dialog);

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == 100)  // Register button clicked
    {
        gtk_widget_destroy(dialog);
        show_register_dialog();
        return;
    }
    else if (res == GTK_RESPONSE_ACCEPT)
    {
        const gchar *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

        if (email && *email && password && *password)
        {
            char response[256];
            if (send_command("LOGIN", email, password, response, sizeof(response)) == 0)
            {
                if (strncmp(response, "OK", 2) == 0)
                {
                    // Parse response: OK|email|username
                    char response_copy[256];
                    strncpy(response_copy, response, sizeof(response_copy) - 1);
                    response_copy[sizeof(response_copy) - 1] = '\0';
                    
                    char *token = strtok(response_copy, "|");
                    token = strtok(NULL, "|");  // email
                    if (token)
                    {
                        strncpy(current_user_email, token, sizeof(current_user_email) - 1);
                        current_user_email[sizeof(current_user_email) - 1] = '\0';
                        token = strtok(NULL, "|");  // username
                        if (token)
                        {
                            strncpy(current_username, token, sizeof(current_username) - 1);
                            current_username[sizeof(current_username) - 1] = '\0';
                        }
                    }
                    update_user_display();
                    on_refresh_groups_clicked(NULL, NULL);
                    if (main_window)
                    {
                        // Show main window after successful login
                        gtk_widget_show_all(main_window);
                        gtk_widget_set_sensitive(main_window, TRUE);
                    }
                }
                else
                {
                    GtkWidget *error_dialog = gtk_message_dialog_new(
                        GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                        "Login failed! Invalid email or password.");
                    gtk_dialog_run(GTK_DIALOG(error_dialog));
                    gtk_widget_destroy(error_dialog);
                    gtk_widget_destroy(dialog);
                    show_login_dialog();  // Show again
                    return;
                }
            }
            else
            {
                GtkWidget *error_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Connection error! Please check if server is running.");
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                gtk_widget_destroy(dialog);
                show_login_dialog();  // Show again
                return;
            }
        }
    }
    else
    {
        // Cancel - show main window with login button if not logged in
        if (current_user_email[0] == '\0')
        {
            if (main_window)
            {
                gtk_widget_show_all(main_window);
                gtk_widget_set_sensitive(main_window, FALSE);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

// Wrapper for button callback
void show_register_dialog_from_button(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    show_register_dialog();
}

// Show register dialog
void show_register_dialog(void)
{

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Register", NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Register",
        GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    // Email entry
    GtkWidget *email_label = gtk_label_new("Email:");
    GtkWidget *email_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email_entry), "user@example.com");
    gtk_grid_attach(GTK_GRID(grid), email_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), email_entry, 1, 0, 1, 1);

    // Username entry
    GtkWidget *username_label = gtk_label_new("Username:");
    GtkWidget *username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Your Name");
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 1, 1, 1);

    // Password entry
    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "password");
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 2, 1, 1);

    gtk_widget_show_all(dialog);

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        const gchar *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
        const gchar *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

        if (email && *email && username && *username && password && *password)
        {
            char response[256];
            // Send: REGISTER, email, password, username
            int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s != -1 && connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0)
            {
                send(s, "REGISTER", COMMAND_LENGTH, 0);
                send(s, email, MAX_FILENAME, 0);
                send(s, password, MAX_FILENAME, 0);
                send(s, username, MAX_FILENAME, 0);
                ssize_t bytes_recv = recv(s, response, sizeof(response) - 1, 0);
                close(s);
                if (bytes_recv > 0)
                {
                    response[bytes_recv] = '\0';
                    if (strncmp(response, "OK", 2) == 0)
                    {
                        // Parse response: OK|email|username
                        char response_copy[256];
                        strncpy(response_copy, response, sizeof(response_copy) - 1);
                        response_copy[sizeof(response_copy) - 1] = '\0';
                        
                        char *token = strtok(response_copy, "|");
                        token = strtok(NULL, "|");  // email
                        if (token)
                        {
                            strncpy(current_user_email, token, sizeof(current_user_email) - 1);
                            current_user_email[sizeof(current_user_email) - 1] = '\0';
                            token = strtok(NULL, "|");  // username
                            if (token)
                            {
                                strncpy(current_username, token, sizeof(current_username) - 1);
                                current_username[sizeof(current_username) - 1] = '\0';
                            }
                        }
                        update_user_display();
                        on_refresh_groups_clicked(NULL, NULL);
                        if (main_window)
                        {
                            // Show main window after successful registration
                            gtk_widget_show_all(main_window);
                            gtk_widget_set_sensitive(main_window, TRUE);
                        }
                        
                        // Show info message
                        GtkWidget *info_dialog = gtk_message_dialog_new(
                            GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                            "Registration successful! You can now create a group or join an existing one.");
                        gtk_dialog_run(GTK_DIALOG(info_dialog));
                        gtk_widget_destroy(info_dialog);
                    }
                    else if (strstr(response, "EXISTS"))
                    {
                        GtkWidget *error_dialog = gtk_message_dialog_new(
                            GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                            "Registration failed! Email already exists.");
                        gtk_dialog_run(GTK_DIALOG(error_dialog));
                        gtk_widget_destroy(error_dialog);
                        gtk_widget_destroy(dialog);
                        show_register_dialog();
                        return;
                    }
                    else
                    {
                        GtkWidget *error_dialog = gtk_message_dialog_new(
                            GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                            "Registration failed! Please try again.");
                        gtk_dialog_run(GTK_DIALOG(error_dialog));
                        gtk_widget_destroy(error_dialog);
                        gtk_widget_destroy(dialog);
                        show_register_dialog();
                        return;
                    }
                }
            }
        }
    }
    else
    {
        // Cancel - show login dialog again
        if (current_user_email[0] == '\0')
        {
            gtk_widget_destroy(dialog);
            show_login_dialog();
            return;
        }
    }

    gtk_widget_destroy(dialog);
}

// Check if current user is leader of a group
int is_current_user_leader(int group_id)
{
    if (current_user_email[0] == '\0' || group_id < 0)
    {
        return 0;
    }
    
    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);
    
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        return 0;
    }
    
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(s);
        return 0;
    }
    
    send(s, "CHECK_LEADER", COMMAND_LENGTH, 0);
    send(s, group_id_str, MAX_FILENAME, 0);
    send(s, current_user_email, MAX_FILENAME, 0);
    
    char response[32];
    ssize_t bytes_recv = recv(s, response, sizeof(response) - 1, 0);
    close(s);
    
    if (bytes_recv > 0)
    {
        response[bytes_recv] = '\0';
        return (strncmp(response, "YES", 3) == 0);
    }
    
    return 0;
}

// Update user display
void update_user_display(void)
{
    if (user_label)
    {
        if (current_user_email[0] != '\0')
        {
            char label_text[256];
            if (current_username[0] != '\0')
            {
                snprintf(label_text, sizeof(label_text), "Logged in as: %s (%s)", current_username,
                        current_user_email);
            }
            else
            {
                snprintf(label_text, sizeof(label_text), "Logged in as: %s", current_user_email);
            }
            gtk_label_set_text(GTK_LABEL(user_label), label_text);
            
            // Show logout button, hide login button
            if (logout_button)
            {
                gtk_widget_show(logout_button);
            }
            if (login_button)
            {
                gtk_widget_hide(login_button);
            }
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(user_label), "Not logged in");
            
            // Show login button, hide logout button
            if (login_button)
            {
                gtk_widget_show(login_button);
            }
            if (logout_button)
            {
                gtk_widget_hide(logout_button);
            }
        }
    }
}

// Logout callback
void on_logout_clicked(GtkButton *button, gpointer user_data)
{
    current_user_email[0] = '\0';
    current_username[0] = '\0';
    current_group_id = -1;
    if (group_list_store)
    {
        gtk_list_store_clear(group_list_store);
    }
    if (main_window)
    {
        gtk_widget_set_sensitive(main_window, FALSE);
    }
    update_user_display();  // This will update label and button visibility
    show_login_dialog();
}

// Callback for create group button
void on_create_group_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Create New Group", GTK_WINDOW(toplevel),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     "_Create", GTK_RESPONSE_ACCEPT, "_Cancel",
                                                     GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter group name...");
    gtk_widget_set_margin_start(entry, 10);
    gtk_widget_set_margin_end(entry, 10);
    gtk_widget_set_margin_top(entry, 10);
    gtk_widget_set_margin_bottom(entry, 10);
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show(entry);

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        const gchar *group_name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (group_name && *group_name)
        {
            if (current_user_email[0] == '\0')
            {
                GtkWidget *error_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Please login first!");
                gtk_dialog_run(GTK_DIALOG(error_dialog));
                gtk_widget_destroy(error_dialog);
                gtk_widget_destroy(dialog);
                return;
            }
            char response[32];
            if (send_command("CREATE_GROUP", group_name, current_user_email, response,
                            sizeof(response)) == 0)
            {
                if (strncmp(response, "OK", 2) == 0)
                {
                    // Refresh group list
                    on_refresh_groups_clicked(NULL, NULL);
                }
                else
                {
                    GtkWidget *error_dialog = gtk_message_dialog_new(
                        GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_OK, "Failed to create group");
                    gtk_dialog_run(GTK_DIALOG(error_dialog));
                    gtk_widget_destroy(error_dialog);
                }
            }
        }
    }

    gtk_widget_destroy(dialog);
}

// Refresh group list
void on_refresh_groups_clicked(GtkButton *button, gpointer user_data)
{
    char response[8192];
    if (send_command("LIST_GROUP", NULL, NULL, response, sizeof(response)) == 0)
    {
        gtk_list_store_clear(group_list_store);

        char *line = strtok(response, "\n");
        while (line != NULL)
        {
            int group_id;
            char group_name[MAX_GROUP_NAME_LEN];
            int member_count;
            if (sscanf(line, "%d|%49[^|]|%d", &group_id, group_name, &member_count) == 3)
            {
                GtkTreeIter iter;
                gtk_list_store_append(group_list_store, &iter);
                char member_str[32];
                snprintf(member_str, sizeof(member_str), "%d members", member_count);
                gtk_list_store_set(group_list_store, &iter, 0, group_name, 1, member_str, -1);
            }
            line = strtok(NULL, "\n");
        }
    }
}

// Open group details window
void open_group_details_window(int group_id, const char *group_name)
{
    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;

    if (gtk_builder_add_from_file(builder, "group_members_screen.glade", &error) == 0)
    {
        g_printerr("Error loading UI file: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(builder);
        return;
    }

    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "group_members_window"));
    GtkWidget *group_name_label =
        GTK_WIDGET(gtk_builder_get_object(builder, "group_name_label"));
    GtkListBox *members_listbox =
        GTK_LIST_BOX(gtk_builder_get_object(builder, "members_listbox"));
    GtkListBox *requests_listbox =
        GTK_LIST_BOX(gtk_builder_get_object(builder, "approval_requests_listbox"));
    GtkWidget *leave_group_button =
        GTK_WIDGET(gtk_builder_get_object(builder, "leave_group_button"));
    GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object(builder, "notebook"));

    char label_text[256];
    snprintf(label_text, sizeof(label_text), "Members of '%s'", group_name);
    gtk_label_set_text(GTK_LABEL(group_name_label), label_text);

    current_group_id = group_id;
    
    // Check if current user is leader
    int is_leader = is_current_user_leader(group_id);
    
    // Load members and requests
    load_members(members_listbox);
    if (is_leader)
    {
        load_requests(requests_listbox);
    }
    else
    {
        // Hide Approval Requests tab if not leader
        if (notebook)
        {
            GtkWidget *requests_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 2);
            if (requests_page)
            {
                gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), 2);
            }
        }
    }

    // Hide leave button if user is leader (leader cannot leave)
    if (is_leader && leave_group_button)
    {
        gtk_widget_set_sensitive(leave_group_button, FALSE);
        gtk_button_set_label(GTK_BUTTON(leave_group_button), "Leave Group (Leader)");
    }

    g_signal_connect(leave_group_button, "clicked", G_CALLBACK(on_leave_group_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), window);

    gtk_widget_show_all(window);
    g_object_unref(builder);
}

// Callback for group row activation
void on_groups_treeview_row_activated(GtkTreeView *treeview, GtkTreePath *path,
                                      GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gchar *group_name;
        gtk_tree_model_get(model, &iter, 0, &group_name, -1);

        // Get group ID
        char response[8192];
        if (send_command("LIST_GROUP", NULL, NULL, response, sizeof(response)) == 0)
        {
            char *line = strtok(response, "\n");
            while (line != NULL)
            {
                int group_id;
                char name[MAX_GROUP_NAME_LEN];
                int member_count;
                if (sscanf(line, "%d|%49[^|]|%d", &group_id, name, &member_count) == 3)
                {
                    if (strcmp(name, group_name) == 0)
                    {
                        open_group_details_window(group_id, group_name);
                        break;
                    }
                }
                line = strtok(NULL, "\n");
            }
        }

        g_free(group_name);
    }
}

// Helper function to create member row
GtkWidget *create_member_row(const gchar *email)
{
    GtkWidget *row, *box, *label;

    row = gtk_list_box_row_new();
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);

    label = gtk_label_new(email);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(row), box);

    return row;
}

// Helper function to create request row (only for leaders)
GtkWidget *create_request_row(int request_id, const gchar *email)
{
    GtkWidget *row, *box, *label, *approve_button;

    row = gtk_list_box_row_new();
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);

    label = gtk_label_new(email);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    approve_button = gtk_button_new_with_label("Approve");
    gtk_box_pack_end(GTK_BOX(box), approve_button, FALSE, TRUE, 0);
    g_signal_connect(approve_button, "clicked", G_CALLBACK(on_approve_request_clicked),
                     GINT_TO_POINTER(request_id));

    gtk_container_add(GTK_CONTAINER(row), box);
    return row;
}

// Load members for current group
void load_members(GtkListBox *members_listbox)
{
    if (current_group_id < 0 || members_listbox == NULL)
    {
        return;
    }

    // Clear existing members
    GList *children = gtk_container_get_children(GTK_CONTAINER(members_listbox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
    {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", current_group_id);

    char response[4096];
    if (send_command("LIST_MEMBERS", group_id_str, NULL, response, sizeof(response)) == 0)
    {
        char *line = strtok(response, "\n");
        while (line != NULL)
        {
            if (strlen(line) > 0)
            {
                GtkWidget *row = create_member_row(line);
                gtk_container_add(GTK_CONTAINER(members_listbox), row);
            }
            line = strtok(NULL, "\n");
        }
        gtk_widget_show_all(GTK_WIDGET(members_listbox));
    }
}

// Load requests for current group (only for leaders)
void load_requests(GtkListBox *requests_listbox)
{
    if (current_group_id < 0 || requests_listbox == NULL)
    {
        return;
    }

    // Check if user is leader
    if (!is_current_user_leader(current_group_id))
    {
        return;  // Only leaders can see requests
    }

    // Clear existing requests
    GList *children = gtk_container_get_children(GTK_CONTAINER(requests_listbox));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
    {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", current_group_id);

    char response[4096];
    if (send_command("LIST_REQUESTS", group_id_str, NULL, response, sizeof(response)) == 0)
    {
        char *line = strtok(response, "\n");
        while (line != NULL)
        {
            if (strlen(line) > 0)
            {
                int request_id;
                char email[MAX_EMAIL_LEN];
                if (sscanf(line, "%d|%29s", &request_id, email) == 2)
                {
                    GtkWidget *row = create_request_row(request_id, email);
                    gtk_container_add(GTK_CONTAINER(requests_listbox), row);
                }
            }
            line = strtok(NULL, "\n");
        }
        gtk_widget_show_all(GTK_WIDGET(requests_listbox));
    }
}

// Callback for request join button (would be in group details window)
void on_request_join_clicked(GtkButton *button, gpointer user_data)
{
    int group_id = GPOINTER_TO_INT(user_data);
    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);

    char response[32];
    if (send_command("REQUEST_JOIN", current_user_email, group_id_str, response,
                    sizeof(response)) == 0)
    {
        if (strncmp(response, "OK", 2) == 0)
        {
            GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
            GtkWidget *info_dialog = gtk_message_dialog_new(
                GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "Request sent successfully");
            gtk_dialog_run(GTK_DIALOG(info_dialog));
            gtk_widget_destroy(info_dialog);
        }
        else
        {
            GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
            GtkWidget *error_dialog = gtk_message_dialog_new(
                GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Failed to send request");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
    }
}

// Callback for join group from context menu
void on_menu_join_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    int group_id = GPOINTER_TO_INT(user_data);
    char group_id_str[32];
    snprintf(group_id_str, sizeof(group_id_str), "%d", group_id);

    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(menuitem));
    char response[32];
    if (send_command("REQUEST_JOIN", current_user_email, group_id_str, response,
                    sizeof(response)) == 0)
    {
        if (strncmp(response, "OK", 2) == 0)
        {
            GtkWidget *info_dialog = gtk_message_dialog_new(
                GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "Request sent successfully");
            gtk_dialog_run(GTK_DIALOG(info_dialog));
            gtk_widget_destroy(info_dialog);
        }
        else
        {
            GtkWidget *error_dialog = gtk_message_dialog_new(
                GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Failed to send request");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        }
    }
}

// Callback for button press events on the TreeView (right-click menu)
gboolean on_groups_treeview_button_press(GtkWidget *treeview, GdkEventButton *event,
                                        gpointer user_data)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint)event->x, (gint)event->y,
                                          &path, NULL, NULL, NULL))
        {
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(model, &iter, path))
            {
                gchar *group_name;
                gtk_tree_model_get(model, &iter, 0, &group_name, -1);

                // Get group ID
                char response[8192];
                int group_id = -1;
                if (send_command("LIST_GROUP", NULL, NULL, response, sizeof(response)) == 0)
                {
                    char *line = strtok(response, "\n");
                    while (line != NULL)
                    {
                        int id;
                        char name[MAX_GROUP_NAME_LEN];
                        int member_count;
                        if (sscanf(line, "%d|%49[^|]|%d", &id, name, &member_count) == 3)
                        {
                            if (strcmp(name, group_name) == 0)
                            {
                                group_id = id;
                                break;
                            }
                        }
                        line = strtok(NULL, "\n");
                    }
                }

                if (group_id > 0)
                {
                    GtkWidget *menu = gtk_menu_new();
                    GtkWidget *item_join = gtk_menu_item_new_with_label("Join Group");
                    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_join);
                    g_signal_connect(item_join, "activate", G_CALLBACK(on_menu_join_activate),
                                     GINT_TO_POINTER(group_id));
                    gtk_widget_show_all(menu);
                    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
                }

                g_free(group_name);
            }
            gtk_tree_path_free(path);
            return TRUE;
        }
    }
    return FALSE;
}

// Callback for approve request button
void on_approve_request_clicked(GtkButton *button, gpointer user_data)
{
    // Check if user is leader
    if (!is_current_user_leader(current_group_id))
    {
        GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
        GtkWidget *error_dialog = gtk_message_dialog_new(
            GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Only group leader can approve requests!");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        return;
    }

    int request_id = GPOINTER_TO_INT(user_data);
    char request_id_str[32];
    snprintf(request_id_str, sizeof(request_id_str), "%d", request_id);

    // Send email to verify leader on server side
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
    {
        return;
    }
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(s);
        return;
    }
    send(s, "APPROVE_REQ", COMMAND_LENGTH, 0);
    send(s, request_id_str, MAX_FILENAME, 0);
    send(s, current_user_email, MAX_FILENAME, 0);
    
    char response[32];
    ssize_t bytes_recv = recv(s, response, sizeof(response) - 1, 0);
    close(s);
    
    if (bytes_recv > 0)
    {
        response[bytes_recv] = '\0';
        if (strncmp(response, "OK", 2) == 0)
        {
            // Find the listboxes in the current window
            GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
            GtkWidget *members_listbox_widget = NULL;
            GtkWidget *requests_listbox_widget = NULL;

            // Traverse widget tree to find listboxes
            GtkWidget *notebook = NULL;
            GList *children = gtk_container_get_children(GTK_CONTAINER(toplevel));
            for (GList *iter = children; iter != NULL; iter = g_list_next(iter))
            {
                GtkWidget *child = GTK_WIDGET(iter->data);
                if (GTK_IS_NOTEBOOK(child))
                {
                    notebook = child;
                    break;
                }
            }
            g_list_free(children);

            if (notebook)
            {
                // Get members tab (page 1)
                GtkWidget *members_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 1);
                if (members_page)
                {
                    GList *page_children = gtk_container_get_children(GTK_CONTAINER(members_page));
                    for (GList *iter = page_children; iter != NULL; iter = g_list_next(iter))
                    {
                        GtkWidget *child = GTK_WIDGET(iter->data);
                        if (GTK_IS_SCROLLED_WINDOW(child))
                        {
                            GtkWidget *scrolled_child = gtk_bin_get_child(GTK_BIN(child));
                            if (scrolled_child && GTK_IS_LIST_BOX(scrolled_child))
                            {
                                members_listbox_widget = scrolled_child;
                            }
                        }
                    }
                    g_list_free(page_children);
                }

                // Get requests tab (page 2)
                GtkWidget *requests_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), 2);
                if (requests_page)
                {
                    GList *page_children = gtk_container_get_children(GTK_CONTAINER(requests_page));
                    for (GList *iter = page_children; iter != NULL; iter = g_list_next(iter))
                    {
                        GtkWidget *child = GTK_WIDGET(iter->data);
                        if (GTK_IS_SCROLLED_WINDOW(child))
                        {
                            GtkWidget *scrolled_child = gtk_bin_get_child(GTK_BIN(child));
                            if (scrolled_child && GTK_IS_LIST_BOX(scrolled_child))
                            {
                                requests_listbox_widget = scrolled_child;
                            }
                        }
                    }
                    g_list_free(page_children);
                }
            }

            // Reload members and requests
            if (members_listbox_widget)
            {
                load_members(GTK_LIST_BOX(members_listbox_widget));
            }
            if (requests_listbox_widget)
            {
                load_requests(GTK_LIST_BOX(requests_listbox_widget));
            }
        }
    }
}

// Callback for leave group button
void on_leave_group_clicked(GtkButton *button, gpointer user_data)
{
    if (current_group_id < 0)
    {
        return;
    }

    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *confirm_dialog = gtk_message_dialog_new(
        GTK_WINDOW(toplevel), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to leave this group?");
    gint res = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
    gtk_widget_destroy(confirm_dialog);

    if (res == GTK_RESPONSE_YES)
    {
        char group_id_str[32];
        snprintf(group_id_str, sizeof(group_id_str), "%d", current_group_id);

        char response[32];
        if (send_command("LEAVE_GROUP", current_user_email, group_id_str, response,
                        sizeof(response)) == 0)
        {
            if (strncmp(response, "OK", 2) == 0)
            {
                current_group_id = -1;
                gtk_widget_destroy(toplevel);
                on_refresh_groups_clicked(NULL, NULL);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GtkWidget *window, *create_group_button, *groups_treeview, *logout_button;
    GtkWidget *header_box;
    GError *error = NULL;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "download_folder/test.glade", &error) == 0)
    {
        g_printerr("Error loading UI file: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    main_window = window;
    create_group_button = GTK_WIDGET(gtk_builder_get_object(builder, "create_group_button"));
    groups_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "groups_treeview"));
    group_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "group_list_store"));

    // Get header box to add user label, login and logout buttons
    header_box = GTK_WIDGET(gtk_builder_get_object(builder, "header_hbox"));
    if (header_box)
    {
        // Create user label
        user_label = gtk_label_new("Not logged in");
        gtk_widget_set_margin_start(user_label, 10);
        gtk_widget_set_margin_end(user_label, 10);
        gtk_box_pack_start(GTK_BOX(header_box), user_label, FALSE, FALSE, 0);

        // Create login button (shown when not logged in)
        login_button = gtk_button_new_with_label("Login");
        gtk_widget_set_margin_start(login_button, 10);
        gtk_widget_set_margin_end(login_button, 10);
        gtk_box_pack_end(GTK_BOX(header_box), login_button, FALSE, FALSE, 0);
        g_signal_connect(login_button, "clicked", G_CALLBACK(show_login_dialog_from_button), NULL);
        gtk_widget_show(login_button);

        // Create logout button (shown when logged in)
        logout_button = gtk_button_new_with_label("Logout");
        gtk_widget_set_margin_start(logout_button, 10);
        gtk_widget_set_margin_end(logout_button, 10);
        gtk_box_pack_end(GTK_BOX(header_box), logout_button, FALSE, FALSE, 0);
        g_signal_connect(logout_button, "clicked", G_CALLBACK(on_logout_clicked), NULL);
        gtk_widget_hide(logout_button);  // Hide initially
    }

    g_signal_connect(create_group_button, "clicked", G_CALLBACK(on_create_group_clicked), NULL);
    g_signal_connect(groups_treeview, "row-activated", G_CALLBACK(on_groups_treeview_row_activated),
                     NULL);
    g_signal_connect(groups_treeview, "button-press-event",
                     G_CALLBACK(on_groups_treeview_button_press), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Don't show main window until login is successful
    // Show login dialog first
    show_login_dialog();

    gtk_main();

    return 0;
}

