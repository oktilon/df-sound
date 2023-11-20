#ifndef VIEW_H
#define VIEW_H

#include <glib.h>
#include <gtk/gtk.h>
#include "app.h"

typedef enum ViewTypeEnum {
    VIEW_TYPE_NONE,
    VIEW_TYPE_REGULAR,
    VIEW_TYPE_BASE,
    VIEW_TYPE_POPUP
} ViewType;

/**
 * @brief Main application mode
 */
typedef enum MainModeEnum {
      MAIN_MODE_NONE
    , MAIN_MODE_DOORS
    , MAIN_MODE_MENU
} MainMode;

/**
 * @brief Door grid data source
 */
typedef enum MainViewTypeEnum {
      MAIN_VIEW_NONE
    , MAIN_VIEW_APARTMENTS
    , MAIN_VIEW_BUSINESSES
} MainViewType;

typedef enum ViewIdEnum {
    viewNone,
    viewMain,
    viewUnit,
    viewMenu,
    viewConnecting,
    viewCallConfirm,
    viewCall,
    viewInformation,
    viewScan,
    viewPin
} ViewId;

#define VIEW_ID_LAST    viewPin

typedef struct ViewStruct {
    ViewId       id;
    gboolean     valid;
    ViewType     type;
    GtkWidget   *window;
    // Handlers
    AppState   (*show)   (AppState old, gpointer data);
    void       (*hide)   ();
    void       (*blur)   ();
} View;

typedef struct ViewsHistoryStruct {
    View                        *view;
    struct ViewsHistoryStruct   *prev;
} ViewsHistory;

View * view_create (ViewId viewId, GtkApplication *app);
gboolean view_draw_user_image (GtkWidget *img, cairo_t *cr, gpointer userData);
const gchar * view_name (ViewId viewId);
const gchar * view_mode (MainMode mode);
const gchar * view_type (MainViewType type);

#endif // VIEW_H