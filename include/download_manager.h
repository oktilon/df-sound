#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib-object.h>

#include "models/mdl_download.h"
#include "models/mdl_doorbell.h"

void download_manager_init (GMainContext * mc);
void download_manager_update_cache (MdlDoorbell *mdl);
GdkPixbuf *download_manager_get_image (const gchar *url);
void download_manager_finished (MdlDownload *load);
void download_manager_dump_cache (void);

#endif // DOWNLOAD_MANAGER_H