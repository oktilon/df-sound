#include "app.h"
#include "download_manager.h"
#include "view_manager.h"

static guint            downloadIndex;
static GData           *imagesCache;
static GMainContext    *mainContext;

static void download_manager_update_cache_item (gpointer item, gpointer _data);
static void download_manager_update_doors_cache (GPtrArray *units);

void download_manager_init (GMainContext * mc) {
    imagesCache = NULL;
    downloadIndex = 0;
    mainContext = mc;
}

const gchar *download_manager_state_name (DownloadState state) {
    switch (state) {
        case DOWNLOAD_INIT:     return "Init";
        case DOWNLOAD_PROCESS:  return "InProc";
        case DOWNLOAD_FINISHED: return "Fin";
    }
    return "Unknown";
}

const gchar *download_manager_image_hash (const gchar *imageUrl) {
    static gchar *ret;
    GChecksum *checkSum = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (checkSum, (guchar*)imageUrl, strlen (imageUrl));
    ret = g_strdup (g_checksum_get_string (checkSum));
    g_checksum_free (checkSum);
    return ret;
}

void download_manager_dump_item (GQuark key, gpointer data, gpointer pnt) {
    gsize *pCnt = pnt;
    DownloadState ds;
    const gchar *url = "none";
    const gchar *state = "Wrong";

    if (MDL_IS_DOWNLOAD (data)) {
        url = download_get_url (data);
        ds = download_get_state (data);
        state = download_manager_state_name (ds);
    }
    GQuark quarkImg = g_quark_from_string (url);
    selfLogDbg ("Rec %u st:%s url:%s [q=%u]", key, state, url, quarkImg);
    (*pCnt)++;
}

void download_manager_dump_cache (void) {
    gsize cnt = 0;

    g_datalist_foreach (&imagesCache, download_manager_dump_item, &cnt);
    selfLogDbg ("Cache has %d records", cnt);
}

void download_manager_finished (MdlDownload *load) {
    g_return_if_fail (MDL_IS_DOWNLOAD (load));
    gboolean ok = FALSE;
    const gchar *name = "NULL";
    gchar *imgFmt = "NULL";
    DownloadState state = download_get_state (load);
    MdlUnit *unit = download_get_unit (load);
    GdkPixbufLoader *loader = download_get_loader (load);
    if(unit) {
        name = unit_get_header (unit);
    }
    if (loader) {
        GdkPixbufFormat *fmt = gdk_pixbuf_loader_get_format (loader);
        if (fmt) {
            imgFmt = gdk_pixbuf_format_get_name (fmt);
            ok = TRUE;
        }
    }
    if (state != DOWNLOAD_FINISHED) return;
    selfLogDbg ("Unit %s image [%s] downloaded [%s] %s\n", name, imgFmt, download_get_url (load), ok ? "OK" : "ERR");

    if (ok && unit) {
        view_manager_set_new_image (unit, gdk_pixbuf_loader_get_pixbuf (loader));
    }
}

// void download_manager_update_members (gpointer units) {
//     guint i, cnt;
//     const gchar *url;
//     MdlDownload *item;
//     GQuark quarkImg;

//     cnt = g_list_model_get_n_items (G_LIST_MODEL (units));
//     for (i = 0; i < cnt; i++) {
//         MdlUnit *unit = (MdlUnit *) g_list_model_get_item (G_LIST_MODEL (units), i);
//         url = unit_get_image_url (unit);
//         if (strlen (url) > 0) {
//             quarkImg = g_quark_from_string (url);

//             if (quarkImg) {
//                 if (!g_datalist_id_get_data (&imagesCache, quarkImg)) {
//                     item = download_new (url, unit, downloadIndex);
//                     downloadIndex++;
//                     g_datalist_id_set_data (&imagesCache, quarkImg, item);

//                     download_start (item, mainContext);
//                 }
//             }
//         }
//     }
// }

static void download_manager_update_cache_item (gpointer item, gpointer _data) {
    const gchar *url;
    MdlDownload *md;
    GQuark quarkImg;
    MdlUnit *unit;

    returnIfFailWrn (MDL_IS_UNIT (item), "Invalid unit pointer");

    unit = MDL_UNIT (item);
    url = unit_get_image_url (unit);
    if (strlen (url) > 0) {
        quarkImg = g_quark_from_string (url);

        if (quarkImg) {
            if (!g_datalist_id_get_data (&imagesCache, quarkImg)) {
                md = download_new (url, unit, downloadIndex);
                downloadIndex++;
                g_datalist_id_set_data (&imagesCache, quarkImg, md);

                download_start (md, mainContext);
            }
        }
    }

    if (unit_has_members (unit)) {
        download_manager_update_doors_cache (unit_get_members (unit));
    }
}

static void download_manager_update_doors_cache (GPtrArray *units) {
    returnIfFailErr (units, "Invalid argument");

    g_ptr_array_foreach (units, download_manager_update_cache_item, NULL);
}

void download_manager_update_cache (MdlDoorbell *mdl) {
    GValue val = G_VALUE_INIT;
    GPtrArray *doors = NULL;
    returnIfFailErr (MDL_IS_DOORBELL (mdl), "Invalid model");

    g_object_get_property (G_OBJECT (mdl), "general-doors", &val);
    doors = (GPtrArray *) g_value_get_pointer (&val);
    download_manager_update_doors_cache (doors);
    g_value_reset (&val);
    doors = NULL;

    g_object_get_property (G_OBJECT (mdl), "business-doors", &val);
    doors = (GPtrArray *) g_value_get_pointer (&val);
    download_manager_update_doors_cache (doors);
    g_value_reset (&val);
    doors = NULL;
}

MdlDownload *download_manager_get_download (const gchar *url) {
    GQuark quarkImg;
    MdlDownload *ret = NULL;
    if (strlen (url) > 0) {
        quarkImg = g_quark_from_string (url);

        if (quarkImg) {
            ret = g_datalist_id_get_data (&imagesCache, quarkImg);
        }
    }
    return ret;
}

GdkPixbuf *download_manager_get_image (const gchar *url) {
    returnValIfFailDbg (strlen (url), NULL, "Empty URL");
    MdlDownload *download = download_manager_get_download (url);
    returnValIfFailErr (MDL_IS_DOWNLOAD (download), NULL, "Download not found [%s]", url);
    DownloadState state = download_get_state (download);
    returnValIfFailWrn (state == DOWNLOAD_FINISHED, NULL, "Download state %s for [%s]", download_manager_state_name (state), url);
    return download_get_image (download);
}