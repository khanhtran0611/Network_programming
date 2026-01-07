#include "mainscreen_utils.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>  // For uint32_t
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For memset
#include <sys/socket.h>
#include <unistd.h>

#include "database/database.h"
#include "util2.h"

void init_main_screen(GtkBuilder *builder)
{
    main_screen = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    create_group_button = GTK_WIDGET(gtk_builder_get_object(builder, "create_group_button"));
    groups_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "groups_treeview"));
    group_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "group_list_store"));
    g_signal_connect(create_group_button, "clicked", G_CALLBACK(on_create_group_clicked), NULL);
    g_signal_connect(groups_treeview, "row-activated", G_CALLBACK(on_groups_treeview_row_activated),
                     NULL);
    g_signal_connect(groups_treeview, "button-press-event",
                     G_CALLBACK(on_groups_treeview_button_press_main), NULL);
    g_signal_connect(main_screen, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}
