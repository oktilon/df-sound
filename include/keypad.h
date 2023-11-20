#ifndef KEYPAD_H
#define KEYPAD_H
#include <gtk/gtk.h>
#include <glib.h>

void keypad_init (GtkBuilder *builder);
gboolean keypad_show ();
gboolean keypad_hide (gboolean clear);
const char *keypad_get_text ();
int keypad_get_text_width ();
void keypad_test ();
void keypad_hide_revealer ();
void keypad_show_revealer ();

#endif // KEYPAD_H