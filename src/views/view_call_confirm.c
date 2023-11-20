#include <glib.h>
#include <math.h>
#include <stdlib.h>

#include "app.h"
#include "common.h"
#include "common.h"
#include "view_manager.h"
#include "download_manager.h"
#include "views/view_call_confirm.h"
#include "models/mdl_unit.h"

#define UI_FILE     "call_confirm.glade"
#define IMAGE_SIZE  200

static GtkWidget    *window;
static GtkWidget    *lblUsername;
static GtkWidget    *lblDescription;
static GtkWidget    *imgUser;
static MdlUnit      *user = NULL;
static GdkPixbuf    *userImage;
static GdkPixbuf    *pictureUser;
static GdkPixbuf    *pictureUnit;
static gpointer      callData;

static gboolean call_confirm_view_on_cancel_clicked (GtkWidget *_w, GdkEventTouch ev);
static gboolean call_confirm_view_on_call_clicked (GtkWidget *_w, GdkEventTouch ev);
static gboolean call_confirm_view_draw_user_image (GtkWidget *img, cairo_t *cr, gpointer userData);

void call_confirm_view_init (GtkApplication *app, View *view) {
    // Variables
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "callConfirmWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnCancelClicked", G_CALLBACK (call_confirm_view_on_cancel_clicked));
    gtk_builder_add_callback_symbol (builder, "btnCallClicked", G_CALLBACK (call_confirm_view_on_call_clicked));
    gtk_builder_add_callback_symbol (builder, "imgUserDraw", G_CALLBACK (call_confirm_view_draw_user_image));
    gtk_builder_connect_signals (builder, NULL);

    // Controls
    lblUsername = GTK_WIDGET (gtk_builder_get_object (builder, "lblUsername"));
    lblDescription = GTK_WIDGET (gtk_builder_get_object (builder, "lblDescription"));
    imgUser = GTK_WIDGET (gtk_builder_get_object (builder, "imgUser"));

    // Update
    gtk_image_set_pixel_size (GTK_IMAGE (imgUser), IMAGE_SIZE);

    // Load resources
    ui_check_resource_image (&pictureUnit, "unit");
    ui_check_resource_image (&pictureUser, "user");

    callData = NULL;


    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_POPUP;
    view->show = call_confirm_view_show;
    view->hide = call_confirm_view_hide;
}

AppState call_confirm_view_show (AppState old, gpointer data) {
    const gchar *header;
    const gchar *desc;
    const gchar *url;
    gboolean isUnit;
    g_return_val_if_fail (window, old);
    g_return_val_if_fail (MDL_IS_UNIT (data), old);

    userImage = NULL;
    callData = data;

    // gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
    // gtk_widget_set_parent (GTK_WIDGET (window), GTK_WIDGET (parent));
    // gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);


    user = MDL_UNIT (data);
    header = unit_get_header (user);
    desc = unit_get_text (user);
    url = unit_get_image_url (user);
    isUnit = unit_has_members (user);

    // Header
    gtk_label_set_label ( GTK_LABEL (lblUsername), header );
    // Description
    if (g_utf8_strlen (desc, -1) > 0) {
        gtk_label_set_label (GTK_LABEL (lblDescription), desc);
        gtk_widget_set_visible (lblDescription, TRUE);
        gtk_widget_set_size_request (lblDescription, 550, 48);
    } else {
        gtk_label_set_label (GTK_LABEL (lblDescription), "");
        gtk_widget_set_visible (lblDescription, FALSE);
        gtk_widget_set_size_request (lblDescription, 550, 0);
    }
    header = NULL;
    desc = NULL;

    // Set User image
    if (strlen (url) > 0) {
        userImage = download_manager_get_image (url);
    }
    if (!userImage) {
        userImage = isUnit ? pictureUnit : pictureUser;
    }
    url = NULL;

    gtk_widget_queue_draw (imgUser);

    gtk_widget_show (window);
    gtk_window_present (GTK_WINDOW (window));

    return UI_CallConfirm;
}

void call_confirm_view_hide () {
    if (!window) return;
    gtk_widget_hide (window);
}

void call_confirm_view_update_image (MdlUnit *u, GdkPixbuf *image) {
    returnIfFailWrn (MDL_IS_UNIT (u), "Invalid unit");
    returnIfFailWrn (user, "View uninitialized yet");
    if (unit_equal_id (u, user)) {
        userImage = image;
        gtk_widget_queue_draw (imgUser);
    }
}

static gboolean call_confirm_view_on_cancel_clicked (GtkWidget *_w, GdkEventTouch ev) {
    selfLogDbg ("Cancel clicked");
    view_manager_return ();
    return FALSE;
}

static gboolean call_confirm_view_on_call_clicked (GtkWidget *_w, GdkEventTouch ev) {
    selfLogDbg ("Call clicked");
    view_manager_call (callData);
    return FALSE;
}

static gboolean call_confirm_view_draw_user_image (GtkWidget *img, cairo_t *cr, gpointer userData) {
    if (userImage) {
        return view_draw_user_image (img, cr, userImage);
    }
    return FALSE;
}