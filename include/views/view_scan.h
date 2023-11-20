#ifndef VIEW_SCAN_H
#define VIEW_SCAN_H

#include <gtk/gtk.h>
#include "app.h"

void scan_view_init (GtkApplication *app, View *view);
AppState scan_view_show (AppState old, gpointer pInformData);
void scan_view_hide ();
void scan_view_reset_countdown ();

#endif // VIEW_SCAN_H