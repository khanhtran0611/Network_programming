#ifndef MAINSCREEN_UTILS_H
#define MAINSCREEN_UTILS_H
#include <gtk/gtk.h>
#include <stdio.h>

extern GtkListStore *group_list_store;
extern GtkListStore *member_list_store;
extern GtkListStore *request_list_store;
extern GtkWidget *create_group_button, *groups_treeview, *logout_button;
extern struct sockaddr_in server_addr2;

int send_command(const char *command, const char *data1, const char *data2, char *response,
                 size_t response_size);
void init_main_screen(GtkBuilder *builder);
void on_refresh_groups_clicked(GtkButton *button, gpointer user_data);
void on_leave_group_clicked(GtkButton *button, gpointer user_data);
void on_approve_request_clicked(GtkButton *button, gpointer user_data);
void load_members(GtkListBox *members_listbox);
void load_requests(GtkListBox *requests_listbox);
void on_logout_clicked(GtkButton *button, gpointer user_data);
void update_user_display(void);
int is_current_user_leader(int group_id);
void on_groups_treeview_row_activated(GtkTreeView *treeview, GtkTreePath *path,
                                      GtkTreeViewColumn *column, gpointer user_data);
gboolean on_groups_treeview_button_press_main(GtkWidget *treeview, GdkEventButton *event,
                                              gpointer user_data);
GtkWidget *create_member_row(const gchar *email);
void on_menu_join_activate(GtkMenuItem *menuitem, gpointer user_data);

#endif