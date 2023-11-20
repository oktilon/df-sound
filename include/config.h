#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

#include "app.h"

const gchar *config_get_language ();
void config_set_language (const gchar *);

#endif // CONFIG_H