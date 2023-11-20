#ifndef MODELS_MDL_DOWNLOAD_H
#define MODELS_MDL_DOWNLOAD_H

#include <stdint.h>
#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include "models/mdl_unit.h"

typedef enum DownloadStateEnum {
    DOWNLOAD_INIT,
    DOWNLOAD_PROCESS,
    DOWNLOAD_FINISHED
} DownloadState;

G_BEGIN_DECLS

#define MDL_TYPE_DOWNLOAD   download_get_type ()
G_DECLARE_FINAL_TYPE (MdlDownload, download, MDL, DOWNLOAD, GObject)

typedef void (*DownloadCallback)(MdlDownload *load);

MdlDownload     *download_new           (const gchar *url
                                        , MdlUnit *unit
                                        , gint id);
void             download_start         (MdlDownload *mdl
                                        , GMainContext *mainContext);

const gchar     *download_get_url       (MdlDownload *mdl);
gint             download_get_id        (MdlDownload *mdl);
MdlUnit         *download_get_unit      (MdlDownload *mdl);
GdkPixbuf       *download_get_image     (MdlDownload *mdl);
GdkPixbufLoader *download_get_loader    (MdlDownload *mdl);
DownloadState    download_get_state     (MdlDownload *mdl);
gboolean         download_is_ready      (gpointer item);

G_END_DECLS

#endif // MODELS_MDL_DOWNLOAD_H