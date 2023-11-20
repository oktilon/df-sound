#ifndef VIEW_CONNECTING_H
#define VIEW_CONNECTING_H

#include <gtk/gtk.h>
#include "app.h"

void connecting_view_init (GtkApplication *app, View *view);
AppState connecting_view_show (AppState old, gpointer data);
void connecting_view_hide ();
gboolean connecting_view_play_dot ();

#endif // VIEW_CONNECTING_H