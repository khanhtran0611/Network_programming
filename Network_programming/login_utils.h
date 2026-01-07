#ifndef LOGIN_UTILS_H
#define LOGIN_UTILS_H
#include <gtk/gtk.h>

extern GtkEntry *login_email;
extern GtkEntry *login_password;
extern GtkCheckButton *show_password_check;
extern GtkButton *login_button, *show_sign_up;
void init_login_screen(GtkBuilder *builder);
void on_login_button_clicked(GtkButton *b, gpointer data);
void on_btn_show_signup_clicked(GtkButton *b, gpointer data);

void on_show_password_toggled(GtkToggleButton *b, gpointer data);

#endif