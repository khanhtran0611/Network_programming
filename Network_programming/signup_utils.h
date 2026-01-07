#ifndef SIGNUP_UTILS_H
#define SIGNUP_UTILS_H
#include <gtk/gtk.h>


extern GtkEntry *signup_email;
extern GtkEntry *signup_username;
extern GtkEntry *signup_pass1;
extern GtkEntry *signup_pass2;
extern GtkCheckButton *show_password_check;
extern GtkButton *signup_button, *cancel_button;

void init_signup_screen(GtkBuilder *builder);
void on_btn_signup_clicked(GtkButton *b, gpointer data);
void on_btn_cancel_signup_clicked(GtkButton *b, gpointer data);

#endif