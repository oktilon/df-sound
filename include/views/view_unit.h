#ifndef VIEW_UNIT_H
#define VIEW_UNIT_H

#include <glib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include "view.h"
#include "models/mdl_doorbell.h"

#define UNIT_GRID_ROW_COUNT         4

void unit_view_init (GtkApplication *app, View *view);
AppState unit_view_show (AppState old, gpointer data);
void unit_view_hide ();
void unit_view_blur ();
void unit_view_new_data (MdlDoorbell *mdl, MainViewType type);
void unit_view_update_image (MdlUnit *unit, GdkPixbuf *image);
// Event handlers
void unit_view_on_close_button_clicked (GtkButton *btn, gpointer data);
void unit_view_on_prev_button_click (GtkButton *_btn, gpointer _data);
void unit_view_on_next_button_click (GtkButton *_btn, gpointer _data);
void unit_view_on_unit_click (GtkFlowBox *flowBoxPtr, GtkFlowBoxChild *child, gpointer _data);

#endif // VIEW_UNIT_H