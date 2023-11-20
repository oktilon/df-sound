#include "app.h"
#include "common.h"
#include "libs/animations.h"
#include "models/mdl_unit.h"
#include "controls/control_grid.h"
#include "download_manager.h"

#include <gtk/gtk.h>
#include <math.h>

// Global resources
static GdkPixbuf               *imgUnit         = NULL;
static GdkPixbuf               *imgUser         = NULL;
static GdkPixbuf               *imgPageActive   = NULL;
static GdkPixbuf               *imgPageInactive = NULL;
static PangoFontDescription    *fontTitle       = NULL;
static PangoFontDescription    *fontDesc        = NULL;
// Swipe gesture variables
static double                   swipeOffset;
static gpointer                 clickItem;
static double                   swipeBegX;
static double                   swipeEnd;
static gboolean                 swipeEnough;
static gboolean                 swipeRollback;
static double                   swipeVelocity;
static gint                     swipePage;
static gint                     swipeDir;
static GdkEventSequence        *swipeSeq;


void control_grid_reset_swipe (double x, GdkEventSequence *seq) {
    clickItem     = NULL;
    swipeSeq      = seq;
    swipeEnough   = FALSE;
    swipeRollback = FALSE;
    swipeBegX     = x;
    swipeOffset   = 0.;
    swipeVelocity = 0.;
    swipeEnd      = 0.;
    swipePage     = 0;
    swipeDir      = 0;
}

void control_grid_get_item_rectangle (guint ix, guint iy, guint p, cairo_rectangle_t *rect) {
    rect->x = GRID_PADDING_LEFT + GRID_WIDTH * p + ix * (GRID_ITEM_WIDTH + GRID_COLUMN_SPACING);
    rect->y = iy * (GRID_ITEM_HEIGHT + GRID_ROW_SPACING);
    rect->width = GRID_ITEM_WIDTH;
    rect->height = GRID_ITEM_HEIGHT;
}

gpointer control_grid_locate_grid_item (GridData *grid, double x, double y, guint p) {
    guint ix, iy, idx;
    gpointer it = NULL;
    cairo_rectangle_t rect;

    returnValIfFailWrn (grid->rowCount == 4, it, "Wrong grid rowCount=%d maxCount=%d", grid->rowCount, grid->maxCount);

    // selfLogTrc ("Locate unit at [%f, %f] on page=%d", x, y, p);

    for (iy = 0; iy < grid->rowCount; iy++) {
        for (ix = 0; ix < GRID_COLUMN_COUNT; ix++) {
            control_grid_get_item_rectangle (ix, iy, 0, &rect);
            if (rect.x <= x && (rect.x + rect.width) >= x && rect.y <= y && (rect.y + rect.height) >= y) {
                idx = p * grid->maxCount + iy * (grid->rowCount - 1) + ix;
                it = g_list_model_get_item (G_LIST_MODEL (grid->model), idx);
                break;
            }
        }
        if (it) break;
    }
    return it;
}

void control_grid_draw_item (GridData *grid, cairo_t *cr, cairo_rectangle_t *rect, gpointer it, GdkPixbuf *image) {
    double degrees = M_PI / 180.0;
    double r = GRID_ITEM_RADIUS;
    double txtLeft = (rect->width - GRID_TEXT_WIDTH) / 2.0;
    double txtTop = GRID_TITLE_TOP;
    double scaleX, scaleY, x, y, imgX, imgY;
    gboolean hasDescription;
    gboolean isUnit;
    cairo_surface_t *surf;
    int layoutHeight;
    GdkPixbuf *src = image;
    GdkPixbuf *dst = NULL;
    MdlUnit *unit;
    int srcH, srcW;
    const gchar *title;
    const gchar *desc;
    const gchar *url;

    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); // FFFFFF
    cairo_new_sub_path (cr);
    cairo_arc (cr, rect->x + rect->width - r, rect->y + r,                r, -90 * degrees,   0 * degrees);
    cairo_arc (cr, rect->x + rect->width - r, rect->y + rect->height - r, r,   0 * degrees,  90 * degrees);
    cairo_arc (cr, rect->x + r,               rect->y + rect->height - r, r,  90 * degrees, 180 * degrees);
    cairo_arc (cr, rect->x + r,               rect->y + r,                r, 180 * degrees, 270 * degrees);
    cairo_close_path (cr);
    cairo_fill (cr);

    returnIfFailWrn (MDL_IS_UNIT (it), "Invalid model for unit!");
    unit = MDL_UNIT (it);

    title = unit_get_header (unit);
    desc = unit_get_text (unit);
    url = unit_get_image_url (unit);
    isUnit = unit_has_members (unit);
    hasDescription = desc ? g_utf8_strlen (desc, -1) > 0 : FALSE;

    if (!src && strlen(url)) {
        src = download_manager_get_image (url);
    }
    if (!src) {
        src = isUnit ? imgUnit : imgUser;
    }

    // Image
    srcH = gdk_pixbuf_get_height (src);
    srcW = gdk_pixbuf_get_width (src);
    dst = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, GRID_IMAGE_SIZE, GRID_IMAGE_SIZE);
    // Eval scale coefficients (to fit source image into itemSize square)
    scaleX = (double)GRID_IMAGE_SIZE / (double)srcW;
    scaleY = (double)GRID_IMAGE_SIZE / (double)srcH;
    // Scale source image into new image
    gdk_pixbuf_scale (src, dst, 0, 0, GRID_IMAGE_SIZE, GRID_IMAGE_SIZE, 0, 0, scaleX, scaleY, GDK_INTERP_BILINEAR);
    // Create Cairo surface from new image
    surf = gdk_cairo_surface_create_from_pixbuf (dst, 1, NULL);
    // Eval circle center in GtkImage
    x = rect->x + rect->width / 2.0;
    y = rect->y + GRID_IMAGE_TOP + GRID_IMAGE_SIZE / 2.0;

    // Draw image background
    cairo_set_source_rgb (cr, 233./255., 231./255., 236./255.); // #E9E7EC  233 231 236
    cairo_arc (cr, x, y, GRID_IMAGE_SIZE / 2.0, 0, 2.0 * M_PI);
    // cairo_fill_preserve (cr);
    cairo_fill (cr);

    cairo_save (cr);

    // Clip image by circle
    imgX = x - GRID_IMAGE_SIZE / 2.0;
    imgY = y - GRID_IMAGE_SIZE / 2.0;
    cairo_set_source_surface (cr, surf, imgX, imgY);
    cairo_arc (cr, x, y, GRID_IMAGE_SIZE / 2.0, 0, 2.0 * M_PI);
    cairo_clip (cr);
    cairo_paint (cr);

    cairo_restore (cr);


    // Title
    cairo_set_source_rgb (cr, 104.0 / 255.0, 107.0 / 255.0, 127.0 / 255.0); // 44474F 104-107-127
    PangoLayout *layout = pango_cairo_create_layout (cr);

    layoutHeight = hasDescription ? GRID_TEXT_HEIGHT : GRID_TITLE_HEIGHT;
    pango_layout_set_text (layout, title, -1);
    pango_layout_set_width (layout, GRID_TEXT_WIDTH * PANGO_SCALE);
    pango_layout_set_height (layout, layoutHeight * PANGO_SCALE);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
    pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    // pango_layout_set_line_spacing (layout, 1.0);

    pango_layout_set_font_description (layout, fontTitle);
    pango_cairo_update_layout (cr, layout);

    if (!hasDescription) {
        if (pango_layout_get_line_count (layout) == 1) {
            txtTop += GRID_TITLE_SPACE / 2.;
        }
    }

    cairo_move_to (cr, rect->x + txtLeft, rect->y + txtTop);
    pango_cairo_show_layout (cr, layout);

    g_object_unref (layout);

    // selfLogTrc ("Draw \"%s\" %s description %s [u:%d, r:%d]", title, hasDescription ? "with" : "without", desc ? desc : "--", unit_get_user (unit), unit_get_room (unit));

    // Description
    if (hasDescription) {
        layout = pango_cairo_create_layout (cr);

        pango_layout_set_width (layout, GRID_TEXT_WIDTH * PANGO_SCALE);
        pango_layout_set_height (layout, GRID_TEXT_HEIGHT * PANGO_SCALE);
        pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
        pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
        pango_layout_set_text (layout, desc, -1);

        pango_layout_set_font_description (layout, fontDesc);
        pango_cairo_update_layout (cr, layout);

        txtTop = GRID_DESC_TOP;

        cairo_move_to (cr, rect->x + txtLeft, rect->y + txtTop);
        pango_cairo_show_layout (cr, layout);

        g_object_unref (layout);
    }

}

gboolean control_grid_draw (GridData *grid, cairo_t *cr) {
    double dx = (-1.0 * GRID_WIDTH * grid->page) + swipeOffset;

    cairo_save (cr);

    cairo_set_source_surface (cr, grid->surface, dx, 0);
    cairo_paint (cr);

    cairo_restore (cr);

    return TRUE;
}

gboolean control_grid_on_grid_draw (GtkWidget *_w, cairo_t *cr, GridData *grid) {
    UNUSED_ARG (_w);
    return control_grid_draw (grid, cr);
}

void control_grid_filter_data (GridData *grid, GListStore *model, const gchar *filter) {
    gboolean ok;
    gpointer it;
    guint i, p, di, ix, iy, add, cnt;
    cairo_rectangle_t rect;

    cnt = g_list_model_get_n_items (G_LIST_MODEL (model));

    // Clear filtered items
    g_list_store_remove_all (grid->model);
    grid->itemCount = 0;

    // Filter Full data into Filtered data
    for (i = 0; i < cnt; i++) {
        ok = FALSE;
        it = g_list_model_get_item (G_LIST_MODEL (model), i);
        if (filter && g_utf8_strlen (filter, -1) > 0) {
            if (MDL_IS_UNIT (it) && unit_contains_text (it, filter)) {
                ok = TRUE;
            }
        } else {
            ok = TRUE;
        }

        if (ok) {
            g_list_store_append (grid->model, it);
            grid->itemCount++;
        }
        g_object_unref (it);
    }


    cnt = g_list_model_get_n_items (G_LIST_MODEL (grid->model));

    // Divide data on pages
    if (cnt > grid->maxCount) {
        add = cnt % grid->maxCount == 0 ? 0 : 1;
        grid->pages = (cnt / grid->maxCount) + add;
        grid->page = 0;
    } else {
        grid->pages = 1;
        grid->page = 0;
    }

    // Delete old surface
    cairo_surface_destroy (grid->surface);

    // Create new grid surface
    grid->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GRID_WIDTH * grid->pages, GRID_HEIGHT);
    cairo_t *cr = cairo_create (grid->surface);

    // Render all items to surface
    p = 0;
    for (i = 0; i < cnt; i++) {
        di = i - p * grid->maxCount;
        if (di >= grid->maxCount) {
            p++;
            di = 0;
        }
        it = g_list_model_get_item (G_LIST_MODEL (grid->model), i);
        ix = di % GRID_COLUMN_COUNT;
        iy = di / GRID_COLUMN_COUNT;
        // selfLogTrc ("Draw item %d-%d [#%d]", ix, iy, di);
        control_grid_get_item_rectangle (ix, iy, p, &rect);
        control_grid_draw_item (grid, cr, &rect, it, NULL);
        g_object_unref (it);
    }

    cairo_destroy (cr);

    gtk_widget_queue_draw (grid->grid);
    gtk_widget_queue_draw (grid->pager);
}

void control_grid_update_item (GridData *grid, gpointer searchItem, GdkPixbuf *image, GEqualFunc comparer) {
    gboolean ok;
    guint p, di, ix, iy, idx;
    gpointer item;
    cairo_rectangle_t rect;

    ok = g_list_store_find_with_equal_func (grid->model, searchItem, comparer, &idx);
    returnIfFailWrn (ok && idx < UINT32_MAX, "Item not found!");

    item = g_list_model_get_item (G_LIST_MODEL (grid->model), idx);
    returnIfFailWrn (item, "Index %d not found!", idx);

    cairo_t *cr = cairo_create (grid->surface);

    p = idx / grid->maxCount;
    di = idx - p * grid->maxCount;

    ix = di % GRID_COLUMN_COUNT;
    iy = di / GRID_COLUMN_COUNT;

    // selfLogTrc ("Redraw item:%d (page=%d, di=%d, ix=%d, iy=%d)", idx, p, di, ix, iy);

    control_grid_get_item_rectangle (ix, iy, p, &rect);
    control_grid_draw_item (grid, cr, &rect, item, image);

    cairo_destroy (cr);

    g_object_unref (item);

    if (p == grid->page) {
        gtk_widget_queue_draw (grid->grid);
    }
}


void control_grid_clear_container (GtkWidget *child, gpointer container) {
    gtk_container_remove (GTK_CONTAINER (container), child);
}

gboolean control_grid_on_pager_draw (GtkWidget *widget, cairo_t *cr, GridData *grid) {
    int allocWidth = gtk_widget_get_allocated_width (widget);
    int cntInactive = grid->pages > 0 ? grid->pages - 1 : 0;
    int width = PAGER_ACTIVE_SIZE + cntInactive * (PAGER_INACTIVE_SIZE + PAGER_SPACE);
    int p, pageTo = -1;
    double rActive = PAGER_ACTIVE_SIZE / 2.;
    double rInactive = PAGER_INACTIVE_SIZE / 2.;
    double deltaR = rActive - rInactive;
    double x;
    double y = rActive;
    double alpha;
    double k = 0.;
    double r;

    if (fabs (swipeOffset) > 0.) {
        if (swipeOffset < 0) {
            pageTo = grid->page + 1;
        } else {
            pageTo = grid->page - 1;
        }
        k = fabs (swipeOffset) / GRID_WIDTH;
    }

    x = (allocWidth - width) / 2.;

    for (p = 0; p < grid->pages; p++) {
        // Inactive base
        r = rInactive;
        ui_set_source_rgb (cr, 0xC4, 0xC6, 0xFF);
        cairo_move_to (cr, x, y);
        cairo_arc (cr, x, y, rInactive, 0, 2 * M_PI);
        cairo_fill (cr);

        // Overdraw active
        alpha = 0.;
        if (pageTo >= 0) {
            if (pageTo == p) {
                alpha = k;
                r = rInactive + alpha * deltaR;
            }
            if (grid->page == p) {
                alpha = 1. - k;
                r = rInactive + alpha * deltaR;
            }
        } else {
            if (p == grid->page) {
                alpha = 1.;
                r = rActive;
            }
        }

        ui_set_source_rgba (cr, 0x00, 0x5A, 0xC1, alpha);
        cairo_move_to (cr, x, y);
        cairo_arc (cr, x, y, r, 0, 2 * M_PI);
        cairo_fill (cr);

        x += PAGER_SPACE + r;
    }

    return TRUE;
}

void control_grid_animation_cb (double value, Animation *a, GridData *grid) {
    swipeOffset = 1. * value * GRID_WIDTH * swipeDir;
    gtk_widget_queue_draw (grid->grid);
    gtk_widget_queue_draw (grid->pager);
}

void control_grid_animation_done (GridData *grid) {
    grid->page += swipePage;
    swipeOffset = 0;
    selfLogTrc ("animation done at page %d", grid->page);
    gtk_widget_queue_draw (grid->grid);
    gtk_widget_queue_draw (grid->pager);

    animation_deinit (grid->animation);
    grid->animation = NULL;
}

void control_grid_animation_start (GridData *grid, double start, double damping, double stiffness, gboolean reverse, double velocity) {
    SpringParams *params;
    guint dur;
    double valBeg = 0.;
    double valEnd = 1.;

    if (damping < 0.) { // Ease
        dur = stiffness;
        grid->animation = timed_animation_new (grid->grid, start, 0., dur, (AnimationValueCallback)control_grid_animation_cb, grid);
        selfLogTrc ("animate ease [beg=%f, end=0.0] dur = %u ms", start, dur);
    } else {
        if (reverse) {
            valBeg = 1.;
            valEnd = 0.;
        }
        params = spring_animation_params_new (damping, 1.0, stiffness);
        grid->animation = spring_animation_new (grid->grid, valBeg, valEnd, params, (AnimationValueCallback)control_grid_animation_cb, grid);
        dur = spring_animation_get_estimated_duration (SPRING_ANIMATION (grid->animation));

        if (start > 0.0) {
            spring_animation_set_offset (SPRING_ANIMATION (grid->animation), start);
        }
        if (velocity > 0.0) {
            spring_animation_set_initial_velocity (SPRING_ANIMATION (grid->animation), velocity);
        }
        selfLogTrc ("animate spring [beg=%f, dam=%f, stiff=%f, vel=%f] est.dur = %u ms", start, damping, stiffness, velocity, dur);
    }

    animation_set_done_handler (grid->animation, (AnimationDoneCallback)control_grid_animation_done);
    animation_start (grid->animation);
}

void control_grid_on_item_click (GridData *grid) {
    if (!clickItem || !MDL_IS_UNIT (clickItem)) return;

    selfLogDbg ("Click on [%p] %s", clickItem, unit_get_header (MDL_UNIT (clickItem)));
    grid->onClick (clickItem);
}

void control_grid_on_swipe (GtkGestureSwipe *self, double velocityX, double _velocityY, GridData *grid) {
    double x, startValue, damping, stiffness, velocity = 0.;
    gboolean enoughVelocity, enoughOffset, reverse = FALSE;

    if (grid->onSwipe)
        (grid->onSwipe) ();

    if (!swipeEnough && fabs(velocityX) < SWIPE_VELOCITY_CLICK) { // Click
        swipeOffset = 0;
        gtk_widget_queue_draw (grid->grid);
        gtk_widget_queue_draw (grid->pager);
        control_grid_on_item_click (grid);
    } else {
        gtk_gesture_get_point (GTK_GESTURE (self), swipeSeq, &x, NULL);
        swipeVelocity = velocityX;
        swipeOffset = x - swipeBegX;
        if (swipeOffset < 0 && grid->page >= (grid->pages - 1)) { // Lock next grid->page
            swipeOffset = swipeOffset / 8.;
            startValue = fabs (swipeOffset) / GRID_WIDTH;
            damping = 1.0;
            stiffness = 200.;
            reverse = TRUE;
            swipeDir = -1;
            velocity = 12.;
        } else if (swipeOffset > 0 && grid->page == 0) { // Lock prev grid->page
            swipeOffset = swipeOffset / 8.;
            startValue = fabs (swipeOffset) / GRID_WIDTH;
            damping = 1.0;
            stiffness = 200.;
            reverse = TRUE;
            swipeDir = 1;
            velocity = 12.;
        } else {
            enoughVelocity = fabs (swipeVelocity) > SWIPE_VELOCITY_ENOUGH;
            enoughOffset = fabs (swipeOffset) > SWIPE_OFFSET_ENOUGH;
            if (enoughVelocity || enoughOffset) { // Enough to change grid->page
                startValue = fabs (swipeOffset) / GRID_WIDTH;
                swipePage = swipeOffset < 0 ? 1 : -1;
                swipeDir = swipeOffset < 0 ? -1 : 1;
                damping = 0.75;
                stiffness = 100.;
                velocity = fabs (velocityX) / 250.;
                if (velocity > 2.5) velocity = 2.5;
            } else {
                startValue = fabs (swipeOffset) / GRID_WIDTH;
                swipeDir = swipeOffset < 0 ? -1 : 1;
                damping = -1.0; // Ease + reverse
                stiffness = fabs (swipeOffset);
                if (stiffness < 50) stiffness = 50;
            }
        }
        control_grid_animation_start (grid, startValue, damping, stiffness, reverse, velocity);
    }

    if (clickItem) g_object_unref (clickItem);
}

/**
 * @brief On start swipe gesture
 *
 * @param self Swipe gesture pointer
 * @param seq
 * @param _data
 */
void control_grid_on_swipe_begin (GtkGesture *self, GdkEventSequence *seq, GridData *grid) {
    // Variables
    double x, y;
    // Stop previous animation, if required
    if (grid->animation) {
        animation_skip (grid->animation);
    }
    // Get touch point
    gtk_gesture_get_point (self, seq, &x, &y);
    // Reset swipe variables
    control_grid_reset_swipe (x, seq);
    // Get item under touch
    clickItem = control_grid_locate_grid_item (grid, x, y, grid->page);
}

/**
 * @brief On move touch point during swipe gesture
 *
 * @param self Swipe gesture pointer
 * @param seq Touch sequence
 * @param _data User data pointer
 */
void control_grid_on_swipe_update (GtkGesture *self, GdkEventSequence *seq, GridData *grid) {
    // Variables
    double velocityX, x;

    // If it's required sequence
    if (seq == swipeSeq) {
        // Get swipe velocity
        gtk_gesture_swipe_get_velocity (GTK_GESTURE_SWIPE (self), &velocityX, NULL);
        // Recognize swipe
        if (fabs (velocityX) > SWIPE_VELOCITY_CLICK) {
            swipeEnough = TRUE;
        }
        // If it's a swipe
        if (swipeEnough) {
            // Get current touch point
            gtk_gesture_get_point (self, seq, &x, NULL);
            // Calc swipe offset
            swipeOffset = x - swipeBegX;
            // Lock out of range swipes
            if (swipeOffset < 0 && grid->page >= (grid->pages - 1)) {
                // Lock last grid->page
                swipeOffset = swipeOffset / 8.;
            } else if (swipeOffset > 0 && grid->page == 0) {
                // Lock first grid->page
                swipeOffset = swipeOffset / 8.;
            }
            // Queue grid to redraw
            gtk_widget_queue_draw (grid->grid);
            gtk_widget_queue_draw (grid->pager);
        }
    }
}

static PangoFontDescription *control_grid_init_font (const gchar *weight, guint size) {
    gchar *fontString = g_strdup_printf ("%s %s %s %d", GRID_TEXT_FONT, GRID_TEXT_STYLE, weight, size);
    PangoFontDescription *fontDesc = pango_font_description_from_string (fontString);
    if (fontDesc) {
        pango_font_description_set_absolute_size(fontDesc, 1. * size * PANGO_SCALE);
    }
    g_free (fontString);
    return fontDesc;
}

void control_grid_set_swipe_callback (GridData *grid, GCallback cb) {
    grid->onSwipe = cb;
}

void control_grid_init (GtkWidget *widget, GtkWidget *pager, GridData *grid, GridItemClickHandler onClick, guint rowCount) {
    cairo_status_t status;

    grid->grid = widget;
    grid->pager = pager;
    grid->onClick = onClick;
    grid->rowCount = rowCount;
    grid->maxCount = rowCount * GRID_COLUMN_COUNT;
    grid->model = g_list_store_new (MDL_TYPE_UNIT);
    grid->itemCount = 0;
    grid->pages = 0;
    grid->page = 0;
    control_grid_reset_swipe (0., NULL);

    grid->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, GRID_WIDTH, GRID_HEIGHT);
    status = cairo_surface_status (grid->surface);
    if (status != CAIRO_STATUS_SUCCESS) {
        selfLogErr ("Surface create error: %d", status);
    }

    ui_check_resource_image (&imgUser, "user");
    ui_check_resource_image (&imgUnit, "unit");
    ui_check_resource_image (&imgPageActive, "page-active");
    ui_check_resource_image (&imgPageInactive, "page-inactive");

    if (!fontTitle) fontTitle = control_grid_init_font (GRID_TITLE_WEIGHT, GRID_TITLE_SIZE);
    if (!fontDesc)  fontDesc  = control_grid_init_font (GRID_DESC_WEIGHT,  GRID_DESC_SIZE);

    GtkGesture *gesture = gtk_gesture_swipe_new (grid->grid);
    if (gesture) {
        g_signal_connect (gesture, "swipe", G_CALLBACK (control_grid_on_swipe), grid);
        g_signal_connect (gesture, "begin", G_CALLBACK (control_grid_on_swipe_begin), grid);
        g_signal_connect (gesture, "update", G_CALLBACK (control_grid_on_swipe_update), grid);
        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_BUBBLE);
        g_object_weak_ref (G_OBJECT (grid->grid), (GWeakNotify) g_object_unref, gesture);
    }

}

void control_grid_destroy (GridData *grid) {
    if (grid->model) {
        g_list_store_remove_all (grid->model);
        g_object_unref (grid->model);
        grid->model = NULL;
    }

    if (grid->surface) {
        cairo_surface_destroy (grid->surface);
    }

}