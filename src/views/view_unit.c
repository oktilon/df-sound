#include <glib.h>
#include <stdlib.h>
#include <math.h>

#include "app.h"
#include "cairo_blur.h"
#include "view_manager.h"
#include "models/mdl_unit.h"
#include "views/view_unit.h"
#include "download_manager.h"
#include "controls/control_grid.h"

#define UI_FILE         "unit.glade"

static GtkApplication   *app;
static GtkWidget        *window;
static GtkWidget        *btnBack = NULL;
static GtkWidget        *lblName;
static GtkWidget        *unitGrid;
static GtkWidget        *pager;
static GtkWidget        *areaOverlay;
static MdlUnit          *unit;
static GridData          grid = {0};
static GListStore       *fullData;
static guint             fullCount;
static GdkPixbuf        *screenShot;
static cairo_surface_t  *screenShotSurface;

static void unit_view_stop_blur ();


void unit_view_display_data () {
    g_list_store_remove_all (fullData);
    fullCount = unit_get_data (unit, fullData);
    control_grid_filter_data (&grid, fullData, "");
}

AppState unit_view_show (AppState old, gpointer data) {
    g_return_val_if_fail (window, old);

    if (data) {
        g_return_val_if_fail (MDL_IS_UNIT (data), old);

        unit = MDL_UNIT (data);
        gtk_label_set_label (GTK_LABEL (lblName), unit_get_header (unit));
        unit_view_display_data ();
    }

    gtk_widget_show (window);
    gtk_window_present (GTK_WINDOW (window));

    unit_view_stop_blur ();

    return UI_UnitView;
}

void unit_view_hide () {
    if (!window) return;
    gtk_widget_hide (window);
    g_list_store_remove_all (fullData);
}

void unit_view_blur () {
    int w = gtk_widget_get_allocated_width (window);
    int h = gtk_widget_get_allocated_height (window);
    GdkWindow *win = gtk_widget_get_window (window);

    if (screenShot) {
        g_object_unref (screenShot);
        screenShot = NULL;
    }

    if (screenShotSurface) {
        cairo_surface_destroy (screenShotSurface);
        screenShotSurface = NULL;
    }

    screenShot = gdk_pixbuf_get_from_window (win, 0, 0, w, h);
    if (screenShot) {
        screenShotSurface = gdk_cairo_surface_create_from_pixbuf (screenShot, 1, NULL);
        _gtk_cairo_blur_surface (screenShotSurface, BLUR_BG_RADIUS);
    } else {
        selfLogErr ("Get window pixbuf error");
    }
    gtk_widget_show (areaOverlay);
    gtk_widget_queue_draw (areaOverlay);
}

static void unit_view_stop_blur () {
    //gtk_image_clear (GTK_IMAGE (areaOverlay));
    gtk_widget_hide (areaOverlay);
}

void unit_view_update_image (MdlUnit *u, GdkPixbuf *image) {
    selfLogDbg ("Got image update for %s", unit_get_header (u));
    returnIfFailWrn (grid.model, "View uninitialized yet");

    control_grid_update_item (&grid, u, image, unit_equal_id);
}

void unit_view_on_close_button_clicked (GtkButton *btn, gpointer data) {
    view_manager_return ();
}

void unit_view_on_item_click (gpointer item) {
    returnIfFailWrn (item && MDL_IS_UNIT (item), "Invalid model!");

    selfLogDbg ("Click on [%p] %s", item, unit_get_header (MDL_UNIT (item)));
    view_manager_call_confirm (MDL_UNIT (item));
}

gboolean unit_view_on_overlay_draw (GtkWidget *widget, cairo_t *cr, gpointer _data) {
    int w, h;
    if (screenShotSurface) {
        w = gtk_widget_get_allocated_width (widget);
        h = gtk_widget_get_allocated_height (widget);
        cairo_set_source_surface (cr, screenShotSurface, 0, 0);
        cairo_paint (cr);
        cairo_set_source_rgba (cr, BLUR_BG_RED, BLUR_BG_GREEN, BLUR_BG_BLUE, BLUR_BG_ALPHA);
        cairo_rectangle (cr, 0, 0, w, h);
        cairo_fill (cr);
        cairo_paint (cr);
    } else {
        selfLogErr ("No surface");
    }

    return FALSE;
}

void unit_view_init (GtkApplication *a, View *view) {
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Store app
    app = a;

    // Get window object
    window = GTK_WIDGET (gtk_builder_get_object (builder, "unitWindow"));
    returnIfFailErr (window, "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (window), app);
    // Set fullscreen
    ui_fullscreen (window);
    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnBackClicked", G_CALLBACK (unit_view_on_close_button_clicked));
    gtk_builder_add_callback_symbol (builder, "viewGridDraw", G_CALLBACK (control_grid_on_grid_draw));
    gtk_builder_add_callback_symbol (builder, "pagerAreaDraw", G_CALLBACK (control_grid_on_pager_draw));
    gtk_builder_add_callback_symbol (builder, "areaOverlayDraw", G_CALLBACK (unit_view_on_overlay_draw));

    // Get objects
    btnBack = GTK_WIDGET (gtk_builder_get_object (builder, "btnBack"));
    lblName = GTK_WIDGET (gtk_builder_get_object (builder, "lblUnitName"));
    unitGrid = GTK_WIDGET (gtk_builder_get_object (builder, "unitGrid"));
    pager = GTK_WIDGET (gtk_builder_get_object (builder, "pagerArea"));
    areaOverlay = GTK_WIDGET (gtk_builder_get_object (builder, "areaOverlay"));

    // Datastore
    fullData = g_list_store_new (MDL_TYPE_UNIT);
    fullCount = 0;

    // Grid
    control_grid_init (unitGrid, pager, &grid, unit_view_on_item_click, UNIT_GRID_ROW_COUNT);
    gtk_builder_connect_signals (builder, &grid);

    // Init View struct
    view->window = window;
    view->valid = TRUE;
    view->type = VIEW_TYPE_BASE;
    view->show = unit_view_show;
    view->hide = unit_view_hide;
    view->blur = unit_view_blur;
}