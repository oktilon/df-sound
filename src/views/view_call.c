#include <glib.h>
#include <stdlib.h>
#include <libintl.h>

#include "app.h"
#include "view_manager.h"
#include "download_manager.h"
#include "models/mdl_unit.h"
#include "views/view_call.h"

#define UI_FILE         "call.glade"
#define STATE_CALLING   "Calling..."
#define STATE_TALK      "Speaking..."

static GtkWidget   *window;
static GtkWidget   *lblUsername;
static GtkWidget   *lblDescription;
static GtkWidget   *imgUser;
static GtkWidget   *lblStatus;
static MdlUnit     *user;
static GdkPixbuf   *userImage;
static GdkPixbuf   *pictureUser;

static void call_view_on_cancel_clicked (GtkButton *btn, gpointer data);
static gboolean call_view_on_user_draw (GtkWidget *widget, cairo_t *cr, gpointer _data);

void call_view_init (GtkApplication *app, View *view) {
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "callingWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);
    // Set fullscreen
    ui_fullscreen (window);

    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnCancelClicked", G_CALLBACK (call_view_on_cancel_clicked));
    gtk_builder_add_callback_symbol (builder, "imgUserDraw", G_CALLBACK (call_view_on_user_draw));

    gtk_builder_connect_signals (builder, NULL);

    // User
    lblUsername = GTK_WIDGET (gtk_builder_get_object (builder, "lblUserName"));
    lblDescription = GTK_WIDGET (gtk_builder_get_object (builder, "lblDesc"));
    imgUser = GTK_WIDGET (gtk_builder_get_object (builder, "imgUser"));
    // Status
    lblStatus = GTK_WIDGET (gtk_builder_get_object (builder, "lblStatus"));

    // Load resources
    ui_check_resource_image (&pictureUser, "user");

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = call_view_show;
    view->hide = call_view_hide;
}

AppState call_view_show (AppState old, gpointer data) {
    // Variables
    const gchar *header;
    const gchar *desc;
    const gchar *url;

    // Test arguments
    g_return_val_if_fail (window, old);
    g_return_val_if_fail (MDL_IS_UNIT (data), old);

    // Get call data
    user = MDL_UNIT (data);
    userImage = NULL;
    header = unit_get_header (user);
    desc = unit_get_text (user);
    url = unit_get_image_url (user);

    // Header
    gtk_label_set_label ( GTK_LABEL (lblUsername), header );

    // Description
    if (g_utf8_strlen (desc, -1) > 0) {
        gtk_label_set_label (GTK_LABEL (lblDescription), desc);
    } else {
        gtk_label_set_label (GTK_LABEL (lblDescription), "");
    }
    header = NULL;
    desc = NULL;

    // Set User image
    if (strlen (url) > 0) {
        userImage = download_manager_get_image (url);
    }
    if (!userImage) {
        userImage = pictureUser;
    }
    url = NULL;
    gtk_widget_queue_draw (imgUser);

    // Set state
    gtk_label_set_label ( GTK_LABEL (lblStatus), _ (STATE_CALLING) );

    gtk_widget_show (window);
    gtk_window_present (GTK_WINDOW (window));

    return UI_Call;
}

void call_view_hide () {
    g_return_if_fail (window);
    gtk_widget_hide (window);
}

static void call_view_on_cancel_clicked (GtkButton *btn, gpointer data) {
    view_manager_stop_call ();
}

static gboolean call_view_on_user_draw (GtkWidget *img, cairo_t *cr, gpointer _data) {
    if (userImage) {
        return view_draw_user_image (img, cr, userImage);
    }
    return FALSE;
}
