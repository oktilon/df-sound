#include <glib.h>

#include "app.h"
#include "view_history.h"

typedef struct ViewHistoryStruct {
    View                        *view;
    struct ViewHistoryStruct    *prev;
} ViewHistory;

static ViewHistory  *first;
static ViewHistory  *last;
static int          size;

void view_history_init () {
    size = 0;
}

void view_history_push (View *view) {
    ViewHistory *cur = (ViewHistory *)g_malloc0 (sizeof (ViewHistory));
    cur->view = view;
    size++;
    if (last) {
        cur->prev = last;
    }
    last = cur;
    if (!first) {
        first = cur;
    }
}

View *view_history_pop () {
    View *ret = NULL;
    if (last) {
        ViewHistory *cur = last->prev;
        ret = last->view;
        g_free (last);
        last = cur;
        size--;
    } else {
        selfLogErr ("Pop from empty history stack!");
    }
    return ret;
}
