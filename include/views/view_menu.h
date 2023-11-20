#ifndef VIEW_MENU_H
#define VIEW_MENU_H

#include <glib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include "view.h"
#include "models/mdl_doorbell.h"


void menu_view_init (GtkApplication *app, View *view);
void menu_view_new_data (MdlDoorbell *mdl);
AppState menu_view_show (AppState old, gpointer data);
void menu_view_hide ();

#endif // VIEW_MENU_H