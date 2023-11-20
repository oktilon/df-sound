#include <glib.h>
#include <stdlib.h>
#include <freetype/freetype.h>
#include <cairo-ft.h>
#include <pango/pango.h>
#include <math.h>

// Timeval
#include <locale.h>
// #include <stdio.h>
#include <sys/time.h>


#include "app.h"
#include "common.h"
#include "keypad.h"
#include "cairo_blur.h"
#include "view_manager.h"
#include "libs/animations.h"
#include "views/view_main.h"
#include "download_manager.h"
#include "controls/control_grid.h"

#define UI_FILE             "main.glade"

static void main_view_on_back_button_clicked (GtkButton *btn, gpointer data);
static void main_view_on_clear_button_click (GtkButton *_btn, gpointer _data);
static void main_view_on_settings_button_click (GtkButton *_btn, gpointer _data);
static void main_view_on_search_swipe (GtkGestureSwipe *_g, double _vx, double _vy, gpointer _data);
static void main_view_on_grid_swipe ();
static gboolean main_view_on_search_focus_in (GtkTextView *_entry, GdkEventFocus _event, gpointer _data);
static gboolean main_view_on_overlay_draw (GtkWidget *widget, cairo_t *cr, gpointer _data);
static gboolean main_view_on_cursor_draw (GtkWidget *widget, cairo_t *cr, gpointer _data);
static void main_view_on_item_click (gpointer item);


static GtkApplication       *app;
static GtkWidget            *winMain;
static GtkWidget            *stackSearch;
static GtkWidget            *btnClose;
static GtkWidget            *imgBack;
static GtkWidget            *imgGear;
static GtkWidget            *pager;
static GtkWidget            *areaOverlay;
static GtkWidget            *lblSearch;
static GtkWidget            *areaCursor;
static Animation            *cursorAnimation;
static double                cursorValue;
static gint                  cursorDelayTimer;
static GtkWidget            *mainGrid = NULL;
static GListStore           *fullData;
static guint                 fullCount;
static GdkPixbuf            *screenShot;
static cairo_surface_t      *screenShotSurface;
static gboolean              searchMode;
static gboolean              showKeypad;
static gboolean              showBackButton;
static GridData              grid = {0};
static mdlDoorbellConfig     doorbellConfig;
static MainViewType          viewType = MAIN_VIEW_APARTMENTS;
GtkWidget                   *eventSearch;

// Debug
GtkWidget                   *boxButton;
GtkWidget                   *boxText;
GtkWidget                   *mainBox;
GtkWidget                   *logoBox;

gboolean main_view_cursor_start ();

void main_view_init (GtkApplication *a, View *view) {
    GtkGesture *swipeSearch;
    // Gtk builder
    GtkBuilder  *builder = gtk_builder_new_from_resource (UI_RESOURCE (UI_FILE));
    returnIfFailErr (builder, "View builder error: %m");

    // Store app
    app = a;

    // Get window object
    winMain = GTK_WIDGET (gtk_builder_get_object (builder, "mainWindow"));
    returnIfFailErr (GTK_IS_WINDOW (winMain), "Get view window error: %m");

    // Set window to app
    gtk_window_set_application (GTK_WINDOW (winMain), app);
    // Set fullscreen
    ui_fullscreen (winMain);
    // Connect signals (glade file)
    gtk_builder_add_callback_symbol (builder, "btnBackClicked", G_CALLBACK (main_view_on_back_button_clicked));
    gtk_builder_add_callback_symbol (builder, "btnSettingsClicked", G_CALLBACK (main_view_on_settings_button_click));
    gtk_builder_add_callback_symbol (builder, "txtSearchFocusIn", G_CALLBACK (main_view_on_search_focus_in));
    // gtk_builder_add_callback_symbol (builder, "btnSearchClicked", G_CALLBACK (main_view_on_search_button_click));
    gtk_builder_add_callback_symbol (builder, "btnClearClicked", G_CALLBACK (main_view_on_clear_button_click));
    gtk_builder_add_callback_symbol (builder, "mainGridDraw", G_CALLBACK (control_grid_on_grid_draw));
    gtk_builder_add_callback_symbol (builder, "pagerAreaDraw", G_CALLBACK (control_grid_on_pager_draw));
    gtk_builder_add_callback_symbol (builder, "areaCursorDraw", G_CALLBACK (main_view_on_cursor_draw));
    gtk_builder_add_callback_symbol (builder, "areaOverlayDraw", G_CALLBACK (main_view_on_overlay_draw));
    // Get objects
    btnClose = GTK_WIDGET (gtk_builder_get_object (builder, "btnClose"));
    imgGear = GTK_WIDGET (gtk_builder_get_object (builder, "imgGear"));
    imgBack = GTK_WIDGET (gtk_builder_get_object (builder, "imgBack"));
    stackSearch = GTK_WIDGET (gtk_builder_get_object (builder, "stackSearch"));
    mainGrid = GTK_WIDGET (gtk_builder_get_object (builder, "mainGrid"));
    pager = GTK_WIDGET (gtk_builder_get_object (builder, "pagerArea"));
    areaOverlay = GTK_WIDGET (gtk_builder_get_object (builder, "areaOverlay"));
    eventSearch = GTK_WIDGET (gtk_builder_get_object (builder, "eventSearch"));
    lblSearch = GTK_WIDGET (gtk_builder_get_object (builder, "lblSearch"));
    areaCursor = GTK_WIDGET (gtk_builder_get_object (builder, "areaCursor"));
    // Debug
    boxButton = GTK_WIDGET (gtk_builder_get_object (builder, "boxButton"));
    boxText = GTK_WIDGET (gtk_builder_get_object (builder, "boxText"));
    mainBox = GTK_WIDGET (gtk_builder_get_object (builder, "mainBox"));
    logoBox = GTK_WIDGET (gtk_builder_get_object (builder, "logoBox"));

    // Datastore
    fullData = g_list_store_new (MDL_TYPE_UNIT);
    fullCount = 0;

    // Grid
    control_grid_init (mainGrid, pager, &grid, main_view_on_item_click, MAIN_GRID_ROW_COUNT);
    control_grid_set_swipe_callback (&grid, main_view_on_grid_swipe);
    gtk_builder_connect_signals (builder, &grid);

    swipeSearch = gtk_gesture_swipe_new (eventSearch);
    g_signal_connect (swipeSearch, "swipe", G_CALLBACK (main_view_on_search_swipe), NULL);
    gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (swipeSearch), GTK_PHASE_BUBBLE);


    // Init keypad
    keypad_init (builder);
    searchMode = FALSE;
    showKeypad = FALSE;
    showBackButton = FALSE;

    // Init View struct
    view->window = winMain;
    view->valid = TRUE;
    view->type = VIEW_TYPE_BASE;
    view->show = main_view_show;
    view->hide = main_view_hide;
    view->blur = main_view_blur;
}


void main_view_filter_data () {
    const gchar *txt = NULL;

    if (searchMode) {
        txt = keypad_get_text ();
    }

    control_grid_filter_data (&grid, fullData, txt);
}

void main_view_on_search_update () {
    gtk_widget_queue_draw (areaCursor);
    main_view_filter_data ();
}

void main_view_new_data (MdlDoorbell *mdl, MainViewType type) {
    guint i, cnt;
    GValue val = G_VALUE_INIT;
    GPtrArray *src = NULL;
    gpointer it;
    const gchar *param;

    selfLogTrc ("Main view new data source : %s", view_type (type));

    // Clear view data storage
    g_list_store_remove_all (fullData);

    // If already selected in menu mode
    if (type != MAIN_VIEW_NONE) {
        // Get required doors source array
        param = type == MAIN_VIEW_BUSINESSES ? "business-doors" : "general-doors";
        g_object_get_property (G_OBJECT (mdl), param, &val);
        src = (GPtrArray *) g_value_get_pointer (&val);
        cnt = ui_ptr_array_length (src);

        // Copy all doors
        selfLogTrc ("Copy %d %s to ListModel", cnt, param);
        for (i = 0; i < cnt; i++) {
            it = g_ptr_array_index (src, i);
            g_list_store_append (fullData, it);
        }
    }

    // Set parameters
    viewType = type;
    doorbellConfig.config = doorbell_get_config (mdl);
    fullCount = g_list_model_get_n_items (G_LIST_MODEL (fullData));
    showBackButton = doorbell_is_menu_mode (mdl);

    // Apply data
    main_view_filter_data ();

    // Update controls
    if (doorbellConfig.installationMode) {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgGear), "defigo-gear", GTK_ICON_SIZE_DIALOG);
    } else {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgGear), "defigo-blank", GTK_ICON_SIZE_DIALOG);
    }
    if (showBackButton) {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgBack), "defigo-back", GTK_ICON_SIZE_DIALOG);
    } else {
        gtk_image_set_from_icon_name (GTK_IMAGE (imgBack), "defigo-blank", GTK_ICON_SIZE_DIALOG);
    }
}

AppState main_view_show (AppState old, gpointer data) {
    if (!winMain) return old;
    gtk_widget_show (winMain);
    gtk_window_present (GTK_WINDOW (winMain));
    main_view_stop_blur ();
    return UI_MainView;
}

void main_view_hide () {
    if (!winMain) return;
    gtk_widget_hide (winMain);
}

void main_view_update_image (MdlUnit *unit, GdkPixbuf *image) {
    selfLogDbg ("Got image update for %s", unit_get_header (unit));
    returnIfFailWrn (grid.model, "View uninitialized yet");

    control_grid_update_item (&grid, unit, image, unit_equal_id);
}

void main_view_cursor_done (gpointer _d) {
    animation_deinit (cursorAnimation);
    cursorAnimation = NULL;
    cursorDelayTimer = g_timeout_add (1, main_view_cursor_start, NULL);
}

void main_view_cursor_value (double value, Animation *_a, gpointer _d) {
    cursorValue = value;
    gtk_widget_queue_draw (areaCursor);
}

gboolean main_view_cursor_start () {
    cursorDelayTimer = 0;
    if (cursorAnimation) return G_SOURCE_REMOVE;
    if (!showKeypad) return G_SOURCE_REMOVE;
    double beg = cursorValue;
    double end = beg < 0.5 ? 1. : 0.;

    cursorAnimation = timed_animation_new (areaCursor, beg, end, 650, main_view_cursor_value, NULL);
    timed_animation_set_easing (TIMED_ANIMATION (cursorAnimation), EASING_EASE_IN_OUT_BACK);
    animation_set_done_handler (cursorAnimation, main_view_cursor_done);
    animation_start (cursorAnimation);

    return G_SOURCE_REMOVE;
}

void main_view_start_search () {
    if (!searchMode) {
        showKeypad = keypad_show ();
        if (showKeypad) {
            gtk_stack_set_visible_child_name (GTK_STACK (stackSearch), "boxText");
            searchMode = TRUE;
            main_view_cursor_start ();
        }
    }
}

void main_view_stop_search (gboolean clean) {
    gboolean hasText = FALSE;
    if (searchMode) {
        hasText = keypad_hide (clean);
        showKeypad = FALSE;
        if (clean || !hasText) {
            gtk_stack_set_visible_child_name (GTK_STACK (stackSearch), "boxButton");
            searchMode = FALSE;
        }
        if (clean) {
            main_view_filter_data ();
        }
        // gtk_widget_grab_focus (flowBox);
    }
}

void main_view_blur () {
    int w = gtk_widget_get_allocated_width (winMain);
    int h = gtk_widget_get_allocated_height (winMain);
    GdkWindow *win = gtk_widget_get_window (winMain);

    selfLogTrc ("start blur %dx%d", w, h);

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
        selfLogTrc ("blur surface");
    } else {
        selfLogErr ("Get window pixbuf error");
    }
    gtk_widget_show (areaOverlay);
    gtk_widget_queue_draw (areaOverlay);
}

void main_view_stop_blur () {
    //gtk_image_clear (GTK_IMAGE (areaOverlay));
    gtk_widget_hide (areaOverlay);
}

static void main_view_on_back_button_clicked (GtkButton *btn, gpointer data) {
    selfLogTrc ("back button (show=%d)", showBackButton);
    if (showBackButton) {
        view_manager_return ();
    }
}

static void main_view_on_clear_button_click (GtkButton *_btn, gpointer _data) {
    main_view_stop_search (TRUE);
    main_view_filter_data ();
}

static void main_view_on_search_swipe (GtkGestureSwipe *_g, double _vx, double _vy, gpointer _data) {
    main_view_start_search ();
}

static void main_view_on_settings_button_click (GtkButton *_btn, gpointer _data) {
    selfLogTrc ("Settings clicked [%s]", doorbellConfig.installationMode ? "ON" : "OFF");
    download_manager_dump_cache ();
    view_manager_debug ();
}

static gboolean main_view_on_search_focus_in (GtkTextView *_entry, GdkEventFocus _event, gpointer _data) {
    main_view_start_search ();
    return FALSE;
}

static void main_view_on_grid_swipe () {
    if (showKeypad) {
        main_view_stop_search (FALSE);
    }
}

static gboolean main_view_on_overlay_draw (GtkWidget *widget, cairo_t *cr, gpointer _data) {
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
        selfLogTrc ("Paint overlay");
    } else {
        selfLogErr ("No surface");
    }

    return FALSE;
}

static gboolean main_view_on_cursor_draw (GtkWidget *widget, cairo_t *cr, gpointer _data) {
    double x = 2. + keypad_get_text_width ();
    double h = 24.;
    double y = (48. - h) / 2.;

    ui_set_source_rgba (cr, 0x00, 0x5A, 0xC1, cursorValue);
    cairo_rectangle (cr, x, y, 2, h);
    cairo_fill (cr);

    return TRUE;
}


static void main_view_on_item_click (gpointer item) {
    returnIfFailWrn (item && MDL_IS_UNIT (item), "Invalid model!");

    selfLogDbg ("Click on [%p] %s", item, unit_get_header (MDL_UNIT (item)));
    if (unit_has_members (MDL_UNIT (item))) {
        view_manager_unit (MDL_UNIT (item));
    } else {
        view_manager_call_confirm (MDL_UNIT (item));
    }
}
