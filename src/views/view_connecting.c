#include <glib.h>
#include <math.h>
#include <stdlib.h>

#include "app.h"
#include "keypad.h"
#include "view_manager.h"
#include "views/view_connecting.h"
#include "libs/animations.h"

#define UI_FILE             "connecting.glade"
#define DOTS_COUNT          3
#define DOT_RADIUS          3.
#define DOT_STEP            10.
#define DOT_JUMP            4.

static GtkWidget   *window;
static GtkWidget   *dotsArea;
static GtkWidget   *lblConnecting;
static Animation   *dotAnimation = NULL;
static double       dotsBaseline;
static guint        dotToPlay;
static guint        dotsTimer = 0;
static double       dots[DOTS_COUNT];

void connecting_view_reset_animation () {
    int i;
    if (dotAnimation) {
        animation_deinit (dotAnimation);
        dotAnimation = NULL;
    }
    if (dotsTimer) {
        g_source_remove (dotsTimer);
        dotsTimer = 0;
    }
    dotToPlay = 0;

    for (i = 0; i < DOTS_COUNT; i++)
        dots[i] = 0.;
}

AppState connecting_view_show (AppState old, gpointer data) {
    if (!window) return old;
    gtk_widget_show_all (window);
    gtk_window_present (GTK_WINDOW (window));

    int h = gtk_widget_get_allocated_height (lblConnecting);
    PangoLayout *l = gtk_label_get_layout (GTK_LABEL (lblConnecting));
    int baseline = pango_layout_get_baseline (l);
    dotsBaseline = baseline / PANGO_SCALE;
    selfLogWrn ("LABEL h=%d, baseline=%f", h, dotsBaseline);

    connecting_view_play_dot ();
    return UI_Connecting;
}

void connecting_view_hide () {
    connecting_view_reset_animation ();
    if (!window) return;
    gtk_widget_hide (window);
}

gboolean connecting_view_on_dots_draw (GtkWidget *widget, cairo_t *cr, gpointer _data) {
    int i;
    double x, y;

    for (i = 0; i < 3; i++) {
        x = DOT_RADIUS + i * DOT_STEP;
        y = dotsBaseline - DOT_RADIUS - (DOT_JUMP * dots[i]);
        ui_set_source_rgb (cr, 0x1B, 0x14, 0x64);
        cairo_move_to (cr, x, y);
        cairo_arc (cr, x, y, DOT_RADIUS, 0, 2 * M_PI);
        cairo_fill (cr);
    }
    return TRUE;
}

void connecting_view_animate_value (double val, Animation *a, gpointer _data) {
    dots[dotToPlay] = val;
    gtk_widget_queue_draw (dotsArea);
}

void connecting_view_animation_done (gpointer _data) {
    guint interval = 1;

    dots[dotToPlay] = 0;
    gtk_widget_queue_draw (dotsArea);

    animation_deinit (dotAnimation);
    dotAnimation = NULL;

    dotToPlay++;
    if (dotToPlay >= DOTS_COUNT) {
        dotToPlay = 0;
        interval = 1000;
    }
    dotsTimer = g_timeout_add (interval, connecting_view_play_dot, NULL);
}

gboolean connecting_view_play_dot () {
    dotsTimer = 0;
    SpringParams *p = spring_animation_params_new_full (120, 1, 10800);
    dotAnimation = spring_animation_new (dotsArea, 0., 1., p, connecting_view_animate_value, NULL);
    animation_set_done_handler (dotAnimation, connecting_view_animation_done);
    animation_start (dotAnimation);

    return G_SOURCE_REMOVE;
}

void connecting_view_init (GtkApplication *app, View *view) {
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "connectingWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);
    // Set fullscreen
    ui_fullscreen (window);
    // Get widgets
    dotsArea = GTK_WIDGET (gtk_builder_get_object (builder, "areaDots"));
    lblConnecting = GTK_WIDGET (gtk_builder_get_object (builder, "lblConnecting"));

    // Connect signals
    gtk_builder_add_callback_symbol (builder, "areaDotsDraw", G_CALLBACK (connecting_view_on_dots_draw));
    gtk_builder_connect_signals (builder, NULL);

    connecting_view_reset_animation ();

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_REGULAR;
    view->show = connecting_view_show;
    view->hide = connecting_view_hide;
}
