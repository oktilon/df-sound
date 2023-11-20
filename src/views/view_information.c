#include <math.h>

#include "app.h"
#include "view_manager.h"
#include "views/view_information.h"

#define UI_FILE             "information.glade"

static GtkWidget           *window;
static GtkWidget           *boxImage;
static GtkWidget           *imgBackground;
static GtkWidget           *imgIcon;
static GtkWidget           *boxMessage;
static GtkWidget           *lblMessage;
static guint                closeTimer;

static guint information_view_set_data (InformationViewData *data, AppState *newState);
static gboolean information_view_close_timeout (gpointer data);

void information_view_init (GtkApplication *app, View *view) {
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "informationWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);

    // Set fullscreen
    ui_fullscreen (window);

    // Get widgets
    boxImage      = GTK_WIDGET (gtk_builder_get_object (builder, "boxImage"));
    imgBackground = GTK_WIDGET (gtk_builder_get_object (builder, "imgBackground"));
    imgIcon       = GTK_WIDGET (gtk_builder_get_object (builder, "imgIcon"));
    boxMessage    = GTK_WIDGET (gtk_builder_get_object (builder, "boxMessage"));
    lblMessage    = GTK_WIDGET (gtk_builder_get_object (builder, "lblMessage"));

    closeTimer = 0;

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = information_view_show;
    view->hide = information_view_hide;
}

AppState information_view_show (AppState old, gpointer pInformData) {
    guint timeout;
    AppState ret = old;

    if (!window) return old;
    if (!pInformData) return old;

    if (closeTimer) {
        g_source_remove (closeTimer);
        selfLogTrc ("Timer %d removed.", closeTimer);
        closeTimer = 0;
    }

    gtk_widget_show_all (window);
    gtk_window_present (GTK_WINDOW (window));

    timeout = information_view_set_data (pInformData, &ret);

    if (timeout) {
        closeTimer = g_timeout_add_seconds (timeout, information_view_close_timeout, NULL);
        selfLogTrc ("Timer %d for %d sec.", closeTimer, timeout);
    }

    return ret;
}

void information_view_update (InformationViewData *pInformData) {
    information_view_show (UI_Information, pInformData);
}

void information_view_hide () {
    returnIfFailWrn(window, "View is not initialized!");
    gtk_widget_hide (window);
}

static guint information_view_set_data (InformationViewData *data, AppState *newState) {
    const gchar *bg_image = "defigo-background-gray";
    const gchar *text_style = "information_error";
    const gchar *text = "";
    const gchar *icon_image = NULL;
    gint bg_top = 184;
    gint text_top = 36;
    gint text_lines = 2;
    GtkStyleContext *styleContext;
    GList *classes;
    guint timeout = 0;

    *newState = UI_Information;

    switch (data->mode) {
    case INF_VIEW_MODE_NO_CONNECTION:
        text = _("Connection to the server interrupted");
        icon_image = "defigo-no-connection";
        *newState = UI_Offline;
        break;

    case INF_VIEW_MODE_KOS_ONLY:
        text = _("Sorry, we’ve lost connection but you can still open your door with your access card");
        bg_image = "defigo-rfid-only";
        text_style = "information_message";
        bg_top = 143;
        text_top = 0;
        text_lines = 3;
        *newState = UI_Offline;
        break;

    case INF_VIEW_MODE_DOOR:
        text = _("Door is opened");
        icon_image = "defigo-unlock";
        text_style = "information_info";
        timeout = 3;
        break;

    case INF_VIEW_MODE_CARD_ERROR:
        text = _("Access card not accepted");
        icon_image = "defigo-card-red";
        timeout = 3;
        break;

    case INF_VIEW_MODE_CARD_ADDED:
        text = _("Your card has been successfully added");
        // bg_image = "defigo-background-green";
        icon_image = "defigo-card-green";
        text_style = "information_success";
        timeout = 3;
        break;

    default:
        break;
    }

    // Background
    gtk_image_set_from_icon_name (GTK_IMAGE (imgBackground), bg_image, GTK_ICON_SIZE_INVALID);
    // gtk_image_set_pixel_size (GTK_IMAGE (imgBackground), 512);
    gtk_widget_set_margin_top (boxImage, bg_top);
    selfLogTrc ("BG [^%d] %s", bg_top, bg_image);

    // Icon
    if (icon_image) {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgIcon), icon_image, GTK_ICON_SIZE_INVALID);
        // gtk_image_set_pixel_size (GTK_IMAGE (imgBackground), 128);
        gtk_widget_set_visible (imgIcon, TRUE);
        selfLogTrc ("ICON %s", icon_image);
    } else {
        gtk_widget_set_visible (imgIcon, FALSE);
        selfLogTrc ("ICON --");
    }

    // Text
    gtk_label_set_text (GTK_LABEL (lblMessage), text);
    styleContext = gtk_widget_get_style_context (lblMessage);
    classes = gtk_style_context_list_classes (styleContext);
    g_list_foreach (classes, (GFunc)ui_remove_class, styleContext);
    gtk_style_context_add_class (styleContext, text_style);
    gtk_widget_set_margin_top (boxMessage, text_top);
    gtk_label_set_lines (GTK_LABEL (lblMessage), text_lines);
    selfLogTrc ("TEXT [^%d, ln:%d, \"%s\"] %s", text_top, text_lines, text_style, text);

    return timeout;
}

static gboolean information_view_close_timeout (gpointer data) {
    view_manager_return ();
    closeTimer = 0;
    return G_SOURCE_REMOVE;
}