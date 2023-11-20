#ifndef VIEW_PIN_H
#define VIEW_PIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include "view.h"
#include "models/mdl_doorbell.h"


void pin_view_init (GtkApplication *app, View *view);
AppState pin_view_show (AppState old, gpointer data);
void pin_view_hide ();
void pin_view_error ();
void pin_view_reset_countdown ();

#endif // VIEW_PIN_H