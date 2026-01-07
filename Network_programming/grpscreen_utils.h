#ifndef GRPSCREEN_UTILS_H
#define GRPSCREEN_UTILS_H
#include <gtk/gtk.h>

#define MAX_FILENAME 256
#define MAX_PATH_LEN 3072
#define COMMAND_LENGTH 16

extern GtkWidget *group_name_label, *members_listbox, *upload_button, *add_folder_button,
    *files_treeview, *folder_back_button;
extern GtkListStore *member_list_store;
extern GtkListStore *request_list_store;
extern GtkListStore *file_list_store;

extern char BASE_PATH[MAX_PATH_LEN];
extern char *root_path;
extern char copied_path[MAX_PATH_LEN + MAX_FILENAME];
extern char copied_type[10];
extern struct sockaddr_in server_addr;

void init_grp_screen(GtkBuilder *builder);
gboolean on_files_treeview_button_press_grp(GtkWidget *treeview, GdkEventButton *event,
                                            gpointer user_data);
// void on_menu_paste_activate(GtkMenuItem *menuitem, gpointer user_data);
char *show_rename_dialog(char *old_filename);
char *check_name_duplicate(int s, char *current_path);
void insert_list_tree(char *filename, char *type, GtkListStore *file_list_store);
void get_list_of_files(GtkListStore *file_list_store);
void on_remove_clicked(GtkButton *button, gpointer user_data);
void on_upload_file_clicked(GtkButton *button, gpointer user_data);
void on_add_folder_clicked(GtkButton *button, gpointer user_data);
void on_folder_back_button_clicked(GtkButton *button, gpointer user_data);
void on_menu_copy_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_download_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_rename_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_view_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_delete_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_paste_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_window_destroy();

#endif