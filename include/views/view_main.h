#ifndef VIEW_MAIN_H
#define VIEW_MAIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include "view.h"
#include "models/mdl_doorbell.h"


#define MAIN_GRID_ROW_COUNT         4

void main_view_init (GtkApplication *app, View *view);
AppState main_view_show (AppState old, gpointer data);
void main_view_hide ();
void main_view_blur ();
void main_view_new_data (MdlDoorbell *mdl, MainViewType type);
void main_view_update_image (MdlUnit *unit, GdkPixbuf *image);
void main_view_on_search_update ();
void main_view_start_search ();
void main_view_stop_search (gboolean clean);
void main_view_stop_blur ();

#endif // VIEW_MAIN_H