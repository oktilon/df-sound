#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <glib-object.h>


#include "app.h"
#include "view.h"
#include "common.h"
#include "download_manager.h"
#include "models/mdl_unit.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif


gboolean view_draw_user_image (GtkWidget *img, cairo_t *cr, gpointer userData) {
    // Variables
    GdkPixbuf *src, *dst;
    GdkWindow *win;
    cairo_surface_t *surf;
    gint itemSize, scale, allocW, allocH, srcH, srcW, dstX, dstY;
    double r, x, y, scaleX, scaleY;

    // Source image
    itemSize = gtk_image_get_pixel_size (GTK_IMAGE (img));
    selfLogDbg ("Draw image on %s [%p] sz=%d", gtk_widget_get_name (img), userData, itemSize);

    // Draw background circle
    ui_set_source_rgb (cr, 0xE9, 0xE7, 0xEC);
    r = itemSize / 2.;
    cairo_move_to (cr, r, r);
    cairo_arc (cr, r, r, r, 0, 2 * M_PI);
    cairo_fill (cr);

    src = (GdkPixbuf *)userData;
    if (src && GDK_IS_PIXBUF (src)) {
        // GdkWindow of GtkImage and it's scale
        win = gtk_widget_get_window (img);
        scale = gtk_widget_get_scale_factor (img);

        // GtkImage real (allocated) size
        allocW = gtk_widget_get_allocated_width (img);
        allocH = gtk_widget_get_allocated_height (img);

        // Reads GtkImage pixel size
        // Get source image size
        srcH = gdk_pixbuf_get_height (src);
        srcW = gdk_pixbuf_get_width (src);
        // Create image of GtkImage size
        dst = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, allocW, allocH);

        // Eval scale coefficients (to fit source image into itemSize square)
        scaleX = (double)itemSize / (double)srcW;
        scaleY = (double)itemSize / (double)srcH;
        // Eval offset to center itemSize square in GtkImage
        dstX = (allocW - itemSize) / 2;
        dstY = (allocH - itemSize) / 2;

        // Scale source image into new image
        gdk_pixbuf_scale (src, dst, dstX, dstY, itemSize, itemSize, dstX, dstY, scaleX, scaleY, GDK_INTERP_BILINEAR);
        // Create Cairo surface from new image
        surf = gdk_cairo_surface_create_from_pixbuf (dst, scale, win);

        // Eval circle center in GtkImage
        x = allocW / 2.0;
        y = allocH / 2.0;

        // Set surface as source for Cairo renderer
        cairo_set_source_surface (cr, surf, 0, 0);
        // Draw itemSize diameter circle in GtkImage center
        cairo_arc (cr, x, y, itemSize / 2.0, 0, 2.0 * M_PI);
        // Clip image by circle
        cairo_clip (cr);
        // Do render
        cairo_paint (cr);

        cairo_surface_destroy (surf);

        return TRUE;
    }

    return FALSE;
}

const gchar * view_name (ViewId viewId) {
    switch (viewId) {
        case viewNone: return "viewNone";
        case viewMain: return "viewMain";
        case viewUnit: return "viewUnit";
        case viewMenu: return "viewMenu";
        case viewConnecting: return "viewConnecting";
        case viewCallConfirm: return "viewCallConfirm";
        case viewCall: return "viewCall";
        case viewInformation: return "viewInformation";
        case viewScan: return "viewScan";
        case viewPin: return "viewPin";
        default: break;
    }
    return "viewUnknown";
}

const gchar * view_mode (MainMode mode) {
    switch (mode) {
        case MAIN_MODE_NONE: return "None";
        case MAIN_MODE_DOORS: return "Doors";
        case MAIN_MODE_MENU: return "Menu";
        default: break;
    }
    return "Wrong";
}

const gchar * view_type (MainViewType type) {
    switch (type) {
        case MAIN_VIEW_NONE: return "None";
        case MAIN_VIEW_APARTMENTS: return "Apartments";
        case MAIN_VIEW_BUSINESSES: return "Businesses";
        default: break;
    }
    return "Wrong";
}