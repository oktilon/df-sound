#ifndef CONTROLS_CONTROL_GRID_H
#define CONTROLS_CONTROL_GRID_H

#include "libs/animations.h"
#include <gtk/gtk.h>

// Grid dimensions
#define GRID_WIDTH                  800
#define GRID_HEIGHT                 980
#define GRID_COLUMN_COUNT           3
#define GRID_PADDING_LEFT           46

// Grid item dimensions
#define GRID_ITEM_WIDTH             220
#define GRID_ITEM_HEIGHT            220
#define GRID_ITEM_RADIUS            24
#define GRID_COLUMN_SPACING         24
#define GRID_ROW_SPACING            24

// Grid item image dimensions
#define GRID_IMAGE_TOP              29
#define GRID_IMAGE_SIZE             80

// Grid item text dimensions
#define GRID_TEXT_WIDTH             174
#define GRID_TEXT_HEIGHT            28
#define GRID_TEXT_FONT             "Poppins"
#define GRID_TEXT_STYLE            "Normal"
#define GRID_TITLE_TOP              125.
#define GRID_TITLE_SIZE             22
#define GRID_TITLE_SPACE            32
#define GRID_TITLE_WEIGHT          "Regular"
#define GRID_TITLE_HEIGHT           66
#define GRID_DESC_TOP               157.
#define GRID_DESC_SIZE              18
#define GRID_DESC_WEIGHT           "Medium"

// Pager sizes
#define PAGER_ACTIVE_SIZE           12
#define PAGER_INACTIVE_SIZE         8
#define PAGER_SPACE                 16

// Swipe parameters
#define SWIPE_VELOCITY_CLICK        100.
#define SWIPE_VELOCITY_ENOUGH       500.
#define SWIPE_VELOCITY_MIN          2000.
#define SWIPE_OFFSET_ENOUGH         (GRID_WIDTH / 2.1)
#define SWIPE_OVERRUN               (GRID_PADDING_LEFT * 0.67)
// Animation Spring
#define SPRING_STIFFNESS            15
#define SPRING_FRAME_RATE           1. / 60.

typedef void (*GridItemClickHandler)(gpointer);

typedef struct GridDataStruct {
    guint                   rowCount;
    guint                   maxCount;
    GtkWidget              *grid;
    GtkWidget              *pager;
    cairo_surface_t        *surface;
    guint                   pages;
    guint                   page;
    GridItemClickHandler    onClick;
    GListStore             *model;
    guint                   itemCount;
    Animation              *animation;
    GCallback               onSwipe;
} GridData;

void control_grid_reset_swipe (double x, GdkEventSequence *seq);
void control_grid_get_item_rectangle (guint ix, guint iy, guint p, cairo_rectangle_t *rect);
gpointer control_grid_locate_grid_item (GridData *grid, double x, double y, guint p);
void control_grid_draw_item (GridData *grid, cairo_t *cr, cairo_rectangle_t *rect, gpointer it, GdkPixbuf *image);
gboolean control_grid_draw (GridData *grid, cairo_t *cr);
gboolean control_grid_on_grid_draw (GtkWidget *widget, cairo_t *cr, GridData *grid);
gboolean control_grid_on_pager_draw (GtkWidget *widget, cairo_t *cr, GridData *grid);
void control_grid_filter_data (GridData *grid, GListStore *model, const gchar *filter);
void control_grid_update_item (GridData *grid, gpointer item, GdkPixbuf *image, GEqualFunc comparer);
void control_grid_on_item_click (GridData *grid);
// Gesture handlers
void control_grid_on_swipe (GtkGestureSwipe *self, double velocityX, double _velocityY, GridData *grid);
void control_grid_on_swipe_begin (GtkGesture *self, GdkEventSequence *seq, GridData *grid);
void control_grid_on_swipe_update (GtkGesture *self, GdkEventSequence *seq, GridData *grid);
// Custom handlers
void control_grid_set_swipe_callback (GridData *grid, GCallback cb);
// Init/deinit grid
void control_grid_init (GtkWidget *widget, GtkWidget *pager, GridData *grid, GridItemClickHandler onClick, guint rowCount);
void control_grid_destroy (GridData *grid);


#endif /* CONTROLS_CONTROL_GRID_H */
