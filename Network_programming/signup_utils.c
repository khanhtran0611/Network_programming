#include "signup_utils.h"

#include <stdio.h>

#include "util2.h"

void init_signup_screen(GtkBuilder* builder)
{
    sign_up_window = GTK_WIDGET(gtk_builder_get_object(builder, "sign_up_window"));
    signup_email = GTK_ENTRY(gtk_builder_get_object(builder, "signup_email"));
    signup_username = GTK_ENTRY(gtk_builder_get_object(builder, "signup_username"));
    signup_pass1 = GTK_ENTRY(gtk_builder_get_object(builder, "signup_pass1"));
    signup_pass2 = GTK_ENTRY(gtk_builder_get_object(builder, "signup_pass2"));
    signup_button = GTK_BUTTON(gtk_builder_get_object(builder, "btn_signup"));
    cancel_button = GTK_BUTTON(gtk_builder_get_object(builder, "btn_cancel"));
    g_signal_connect(signup_button, "clicked", G_CALLBACK(on_btn_signup_clicked), NULL);
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_btn_cancel_signup_clicked), NULL);
}

void on_btn_signup_clicked(GtkButton* b, gpointer data) {}

void on_btn_cancel_signup_clicked(GtkButton* b, gpointer data) {}
