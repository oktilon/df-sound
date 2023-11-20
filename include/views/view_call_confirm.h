#ifndef VIEW_CALL_CONFIRM_H
#define VIEW_CALL_CONFIRM_H

#include <gtk/gtk.h>
#include "app.h"
#include "view.h"

void call_confirm_view_init (GtkApplication *app, View *view);
AppState call_confirm_view_show (AppState old, gpointer data);
void call_confirm_view_hide ();
void call_confirm_view_update_image (MdlUnit *u, GdkPixbuf *image);

#endif // VIEW_CALL_CONFIRM_H