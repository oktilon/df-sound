#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <libintl.h>

#include "app.h"
#include "view_manager.h"
#include "views/view_scan.h"
#include "libs/animations.h"

#define UI_FILE             "scan.glade"
#define SCAN_TIMEOUT        30

static GtkWidget           *window;
static GtkWidget           *lblCountdown;
static GtkWidget           *areaAnimation;
static Animation           *scanAnimation;
static double               animationValue;
static time_t               startTime;
static uint                 countDown;
static GdkPixbuf           *imgSymbol;
static gboolean             skipExit;

static void scan_view_on_close_click (GtkWidget *widget, gpointer _data);
static gboolean scan_view_on_area_draw (GtkWidget *widget, cairo_t *cr, gpointer _data);
static void scan_view_animate_value (double val, Animation *a, gpointer _data);
static void scan_view_animation_done (gpointer _data);

void scan_view_init (GtkApplication *app, View *view) {
    guint duration;
    guint repeatCount;
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "scanWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);

    // Set fullscreen
    ui_fullscreen (window);

    // Get widgets
    lblCountdown  = GTK_WIDGET (gtk_builder_get_object (builder, "lblCountdown"));
    areaAnimation = GTK_WIDGET (gtk_builder_get_object (builder, "areaAnimation"));

    // Connect signals
    gtk_builder_add_callback_symbol (builder, "btnCloseClicked", G_CALLBACK (scan_view_on_close_click));
    gtk_builder_add_callback_symbol (builder, "areaAnimationDraw", G_CALLBACK (scan_view_on_area_draw));
    gtk_builder_connect_signals (builder, NULL);

    ui_check_resource_image (&imgSymbol, "wifi");

    duration = 1000; // sec
    scanAnimation = timed_animation_new (areaAnimation, 0, 1, duration, scan_view_animate_value, NULL);
    repeatCount = SCAN_TIMEOUT * (1000 / duration);
    timed_animation_set_repeatCount (TIMED_ANIMATION (scanAnimation), repeatCount);
    animation_set_done_handler (scanAnimation, scan_view_animation_done);
    startTime = 0;
    skipExit = FALSE;

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = scan_view_show;
    view->hide = scan_view_hide;
}

AppState scan_view_show (AppState old, gpointer _data) {
    if (!window) return old;

    gtk_label_set_text (GTK_LABEL (lblCountdown), "");
    gtk_widget_show_all (window);
    gtk_window_present (GTK_WINDOW (window));

    startTime = time (NULL);
    countDown = 0;
    skipExit = FALSE;

    animation_start (scanAnimation);

    return UI_CardAdd;
}

void scan_view_hide () {
    returnIfFailWrn(window, "View is not initialized!");
    skipExit = TRUE;
    animation_skip (scanAnimation);
    gtk_widget_hide (window);
}

void scan_view_reset_countdown () {
    startTime = time (NULL);
    skipExit = TRUE;
    animation_skip (scanAnimation);
    animation_start (scanAnimation);
    skipExit = FALSE;
    selfLogTrc ("new timeout");
}

static void scan_view_on_close_click (GtkWidget *_widget, gpointer _data) {
    animation_skip (scanAnimation);
}

static gboolean scan_view_on_area_draw (GtkWidget *widget, cairo_t *cr, gpointer _data) {
    cairo_matrix_t saveMatrix;
    double y = 300. + fabs (0.5 - animationValue) * 100.;
    ui_set_source_rgb (cr, 0xD8, 0xE2, 0xFF);

    cairo_get_matrix (cr, &saveMatrix);
    cairo_translate (cr, 400, y);
    cairo_scale (cr, 1, 0.6475); // 518 / 800
    cairo_translate (cr, -400, -y);
    cairo_new_path (cr);
    cairo_arc (cr, 400, y, 400, 0, 2 * M_PI);
    cairo_set_matrix (cr, &saveMatrix);
    cairo_fill (cr);

    gdk_cairo_set_source_pixbuf (cr, imgSymbol, 400 - 128/2, 80);
    cairo_paint (cr);

    return FALSE;
}

static void scan_view_animate_value (double val, Animation *a, gpointer _data) {
    animationValue = val;
    gtk_widget_queue_draw (areaAnimation);
    uint cnt = SCAN_TIMEOUT - (time (NULL) - startTime);

    if (cnt != countDown && cnt >= 0) {
        countDown = cnt;
        gchar *txt = g_strdup_printf (_("%d sec left"), countDown);
        gtk_label_set_text (GTK_LABEL (lblCountdown), txt);
        g_free (txt);
    }
}

static void scan_view_animation_done (gpointer _data) {
    if (skipExit) return;
    view_manager_return ();
}