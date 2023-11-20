#ifndef VIEW_HISTORY_H
#define VIEW_HISTORY_H

#include <glib.h>
#include <gtk/gtk.h>

#include "view.h"

void view_history_init ();
void view_history_push (View *view);
View *view_history_pop ();

#endif // VIEW_HISTORY_H