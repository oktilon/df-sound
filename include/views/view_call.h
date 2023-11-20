#ifndef VIEW_CALL_H
#define VIEW_CALL_H

#include <gtk/gtk.h>
#include "app.h"
#include "view.h"

void call_view_init (GtkApplication *app, View *view);
AppState call_view_show (AppState old, gpointer data);
void call_view_hide ();
gboolean iew_call_confirm_on_clicked_cancel (GtkWidget *_w, GdkEventTouch ev);

#endif // VIEW_CALL_H