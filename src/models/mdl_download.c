#include <curl/curl.h>

#include "models/mdl_download.h"
#include "download_manager.h"
#include "app.h"

/**
 * @brief Door data class
 */
typedef struct _MdlDownloadPrivate {
    gint id;
    const gchar *url;
    GdkPixbufLoader *loader;
    guint64 roomId;
    guint64 userId;
    const gchar *name;
    DownloadState state;
    guint idleId;
    GMainContext *mainContext;
    // DME_Source *source;
    // GMutex mutex;
} MdlDownloadPrivate;

/**
 * @brief Doorbell data parsed for services
 */
struct _MdlDownload {
    GObject parent_instance;
};

enum {
    PROP_0,
    PROP_ID,
    PROP_URL,
    PROP_IMAGE,
    PROP_STATE,

    N_PROPERTIES
};

// extern GMutex        mainMutex;

static GParamSpec   *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (MdlDownload, download, G_TYPE_OBJECT)

#define GET_PRIVATE(d) ((MdlDownloadPrivate *) download_get_instance_private ((MdlDownload*)d))

static void download_init (MdlDownload *mdl) {
    MdlDownloadPrivate *priv = GET_PRIVATE (mdl);

    priv->state = DOWNLOAD_INIT;
    priv->idleId = 0;
    // priv->emitted = FALSE;

    // selfLogTrc ("Download init! %s", priv->url);
}

static void download_finalize (GObject *object) {
    MdlDownloadPrivate *priv = GET_PRIVATE (object);
    // selfLogTrc ("Finalize download %s!", priv->url);
    if (priv->idleId) g_source_remove (priv->idleId);

    G_OBJECT_CLASS (download_parent_class)->finalize (object);
}

static void download_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    MdlDownloadPrivate *priv;
    GdkPixbuf *img;

    g_return_if_fail (MDL_IS_DOWNLOAD (object));
    priv = GET_PRIVATE (object);

    switch (prop_id) {
        case PROP_ID:
            g_value_set_int (value, priv->id);
            break;

        case PROP_URL:
            g_value_set_string (value, priv->url);
            break;

        case PROP_IMAGE:
            img = gdk_pixbuf_loader_get_pixbuf (priv->loader);
            g_value_set_pointer (value, img);
            break;

        case PROP_STATE:
            g_value_set_uint (value, priv->state);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void download_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    g_return_if_fail (MDL_IS_DOWNLOAD (object));

    switch (prop_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void download_class_init (MdlDownloadClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = download_finalize;
    object_class->set_property = download_set_property;
    object_class->get_property = download_get_property;

    properties[PROP_ID] = g_param_spec_int ("id", "Id", "Download id", INT32_MIN, INT32_MAX, -1, G_PARAM_READABLE);
    properties[PROP_URL] = g_param_spec_string ("url", "Url", "Download url", "", G_PARAM_READABLE);
    properties[PROP_IMAGE] = g_param_spec_pointer ("image", "Image", "Downloaded image", G_PARAM_READABLE);
    properties[PROP_STATE] = g_param_spec_uint ("state", "State", "Download state", 0, UINT32_MAX, 0, G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* Public API */

MdlDownload *download_new (const gchar *url, MdlUnit *unit, gint id) {
    // Variables
    gchar *file = strrchr (url, '/');
    selfLogTrc ("Download new (%s)", file ? file : url);
    MdlDownloadPrivate *priv;
    MdlDownload *mdl = (MdlDownload *) g_object_new (MDL_TYPE_DOWNLOAD, NULL);

    if (!(MDL_IS_DOWNLOAD (mdl))) {
        selfLogErr ("Create model error: %m");
        return NULL;
    }
    priv = GET_PRIVATE (mdl);

    priv->id = id;
    priv->url = g_strdup (url);
    priv->roomId = unit_get_room (unit);
    priv->userId = unit_get_user (unit);
    priv->name = g_strdup (unit_get_header (unit));
    priv->loader = gdk_pixbuf_loader_new ();
    return mdl;
}

const gchar *download_get_url (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), "");
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->url;
}

gint download_get_id (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), 0);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->id;
}

MdlUnit *download_get_unit (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), 0);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return unit_new (priv->roomId, priv->userId, priv->name);
}

GdkPixbuf *download_get_image (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), NULL);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Check status
    if (priv->state != DOWNLOAD_FINISHED) return NULL;
    if (!priv->loader) return NULL;
    // Return value
    return gdk_pixbuf_loader_get_pixbuf (priv->loader);
}

GdkPixbufLoader *download_get_loader (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), 0);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->loader;
}

DownloadState download_get_state (MdlDownload *mdl) {
    // Variable
    MdlDownloadPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (mdl), DOWNLOAD_INIT);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->state;
}

gboolean download_is_ready (gpointer item) {
    // Check model
    g_return_val_if_fail (MDL_IS_DOWNLOAD (item), FALSE);
    // Variable
    MdlDownloadPrivate *priv = GET_PRIVATE (MDL_DOWNLOAD (item));
    // Return value
    return priv->state == DOWNLOAD_FINISHED; // && !priv->emitted;
}

size_t download_write_chunk (guchar *ptr, size_t size, size_t nmemb, gpointer userdata) {
    g_return_val_if_fail (MDL_IS_DOWNLOAD (userdata), 0);
    GError *err = NULL;
    MdlDownloadPrivate *priv = GET_PRIVATE (userdata);
    size_t sz = size * nmemb;
    gboolean ok = gdk_pixbuf_loader_write (priv->loader, ptr, sz, &err);
    // selfLogTrc ("LOAD(%d) Write chunk %lu %s", priv->id, sz, ok ? "OK" : "Err");
    if(!ok) {
        selfLogErr ("LOAD(%d) Write chunk error %s", priv->id, err->message);
    }
    return ok ? sz : 0;
}

gboolean download_callback (gpointer data) {
    download_manager_finished ((MdlDownload *)data);
    (GET_PRIVATE (MDL_DOWNLOAD (data)))->idleId = 0;
    return FALSE;
}

gpointer download_thread (gpointer data) {
    g_return_val_if_fail (MDL_IS_DOWNLOAD (data), NULL);
    CURL *curl;
    CURLcode ret;
    MdlDownloadPrivate *priv = GET_PRIVATE (data);
    // GSource *cbSource;

    // g_mutex_lock (&(priv->mutex));
    curl = curl_easy_init();
    if(curl) {

        // epoll_ctl (priv->source->pollfd.fd, EPOLL_CTL_ADD, curl)
        curl_easy_setopt(curl, CURLOPT_URL, priv->url);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_chunk);

        ret = curl_easy_perform(curl);
        (void)ret;
        // selfLogTrc ("CURL ret = %d", ret);

        /* always cleanup */
        curl_easy_cleanup(curl);

        priv->state = DOWNLOAD_FINISHED;

        priv->idleId = gdk_threads_add_idle_full (G_PRIORITY_DEFAULT, download_callback, data, NULL);
        // u = g_idle_add (download_callback, data);

        // cbSource = g_idle_source_new ();

        // g_mutex_lock (&mainMutex);
        // g_source_set_callback (cbSource, download_callback, data, NULL);
        // u = g_source_attach (cbSource, priv->mainContext);
        // g_mutex_unlock (&mainMutex);

        // selfLogTrc ("Idle added: %u", priv->idleId);
    }
    // g_mutex_unlock (&(priv->mutex));

    return NULL;
}

void download_start (MdlDownload *mdl, GMainContext *mainContext) {
    // Variables
    MdlDownloadPrivate *priv;
    GError *err = NULL;
    GThread *th;
    // Check model
    g_return_if_fail (MDL_IS_DOWNLOAD (mdl));
    // Get private
    priv = GET_PRIVATE (mdl);
    priv->state = DOWNLOAD_PROCESS;
    priv->mainContext = mainContext;
    // priv->source = (DME_Source *)source;

    // gchar *name = g_strdup_printf ("loader-%d", priv->id);

    th = g_thread_try_new (NULL, download_thread, mdl, &err);
    if (!th) {
        selfLogErr ("Start download #%d error: %s", priv->id, err ? err->message : "Unknown error");
    // } else {
        // selfLogTrc ("Started download #%d [%s]", priv->id, priv->url);
    }
}