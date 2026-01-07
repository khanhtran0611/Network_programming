#include "login_utils.h"

#include <stdio.h>

#include "util2.h"

void init_login_screen(GtkBuilder* builder)
{
    login_window = GTK_WIDGET(gtk_builder_get_object(builder, "login_window"));
    login_email = GTK_ENTRY(gtk_builder_get_object(builder, "email_entry"));
    login_password = GTK_ENTRY(gtk_builder_get_object(builder, "password_entry"));
    show_password_check = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "show_password_check"));
    login_button = GTK_BUTTON(gtk_builder_get_object(builder, "login_button"));
    show_sign_up = GTK_BUTTON(gtk_builder_get_object(builder, "btn_show_signup"));
}

void on_login_button_clicked(GtkButton* b, gpointer data) {}

void on_btn_show_signup_clicked(GtkButton* b, gpointer data) {}

void on_show_password_toggled(GtkToggleButton* b, gpointer data) {}
