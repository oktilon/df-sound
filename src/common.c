#include "app.h"
#include "common.h"

void ui_check_resource_image (GdkPixbuf **img, const gchar *name) {
    GError *err = NULL;

    if (!(*img)) {
        gchar *file = g_strdup_printf (UI_RESOURCE ("defigo-%s.png"), name);
        *img = gdk_pixbuf_new_from_resource (file, &err);
        if (err) {
            selfLogErr ("Load %s image error: %s", name, err->message);
            *img = NULL;
        }
    }
}

inline void ui_set_source_rgb (cairo_t *cr, int r, int g, int b) {
    cairo_set_source_rgb (cr, r / 255., g / 255., b / 255.);
}

inline void ui_set_source_rgba (cairo_t *cr, int r, int g, int b, double alpha) {
    cairo_set_source_rgba (cr, r / 255., g / 255., b / 255., alpha);
}

inline void ui_remove_class (const gchar *class, GtkStyleContext *context) {
    gtk_style_context_remove_class (context, class);
    selfLogTrc ("removed class %s [%p]", class, class);
}

static inline void ptr_array_iterator (gpointer _it, uint *cnt) { (*cnt)++; }

inline uint ui_ptr_array_length (GPtrArray *arr) {
    uint cnt = 0;
    g_ptr_array_foreach (arr, (GFunc) ptr_array_iterator, &cnt);
    return cnt;
}

void debug_alloc_size (GtkWidget *w, const char *name) {
    GtkAllocation alloc = {0};
    gtk_widget_get_allocated_size (w, &alloc, NULL);
    selfLogWrn ("%s at: (%d, %d) size: %d x %d", name, alloc.x, alloc.y, alloc.width, alloc.height);
}