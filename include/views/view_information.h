#ifndef VIEW_INFORMATION_H
#define VIEW_INFORMATION_H

#include <gtk/gtk.h>
#include "app.h"

typedef enum InformationViewModeEnum {
    INF_VIEW_MODE_NONE,
    INF_VIEW_MODE_NO_CONNECTION,
    INF_VIEW_MODE_KOS_ONLY,
    INF_VIEW_MODE_DOOR,
    INF_VIEW_MODE_CARD_ERROR,
    INF_VIEW_MODE_CARD_ADDED
} InformationViewMode;

typedef struct InformationViewDataStruct {
    InformationViewMode mode;
    const char *text;
} InformationViewData;

void information_view_init (GtkApplication *app, View *view);
AppState information_view_show (AppState old, gpointer pInformData);
void information_view_update (InformationViewData *pInformData);
void information_view_hide ();

#endif // VIEW_INFORMATION_H