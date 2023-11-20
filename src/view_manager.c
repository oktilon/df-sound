#include <glib.h>
#include <gdk/gdkwayland.h>

#include "app.h"
#include "dbus.h"
#include "view.h"
#include "download_manager.h"
#include "view_manager.h"

typedef struct ViewManagerStackItemStruct {
    ViewId id;
    ViewType type;
} ViewManagerStackItem;

MainMode                 mainMode = MAIN_MODE_NONE;
static GtkApplication   *app;
static View             *activeView;
static AppState          state        = UI_Initializing;
static MainViewType      mainViewType = MAIN_VIEW_NONE;
static MdlDoorbell      *mainData;
static gulong            selectedUserId  = 0UL;
static gulong            selectedRoomId  = 0UL;
static gboolean          hasConnectedKos = FALSE;
static gulong            rfidCard     = 0UL;
static long              rfidUserId   = 0L;

static GQueue           *queueUnits;

static View *views[VIEW_ID_LAST + 1] = {0};

// Internal definitions
static View * view_manager_create_view (ViewId viewId);
static void view_manager_enter (ViewId viewId, gpointer data);
static int view_manager_show (ViewId viewId, gpointer data);
static gboolean view_manager_update_unit ();
static View *view_manager_get (ViewId viewId);
static void view_manager_set_state (AppState newState);
static void view_manager_reset_stack ();
static void view_manager_stack_active_view ();
// static void view_manager_pop_stack ();
static const gchar *view_manager_state_name (AppState state);
static const gchar *view_manager_gw_status_name (GatewayStatus state);
static void view_manager_debug_item (ViewManagerStackItem *it, int *pCnt);

void view_manager_init (GtkApplication *_app) {
    selfLogDbg ("View manager init...");
    // Remember app
    app = _app;

    mainData = doorbell_new (NULL);

    GtkCssProvider *css = gtk_css_provider_new ();
    gtk_css_provider_load_from_resource (css, UI_RESOURCE ("window.css"));

    GdkScreen *screen = gdk_screen_get_default ();
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    const gchar *backend = "Unknown";
    GdkDisplay *display = gdk_display_get_default ();

    if (GDK_IS_WAYLAND_DISPLAY(display))
        backend = "Wayland";

    selfLogInf ("Display backend: %s", backend);

    queueUnits = g_queue_new ();
}

void view_manager_set_new_data (MdlDoorbell *mdl) {
    GValue id = G_VALUE_INIT;
    GValue bid = G_VALUE_INIT;
    GValue cntGeneral = G_VALUE_INIT;
    GValue cntBusiness = G_VALUE_INIT;
    MainViewType viewType = mainViewType;
    MainMode newMode = MAIN_MODE_NONE;
    ViewId newViewId = viewNone;
    gboolean newState = FALSE;
    guint queueSize;
    View *view;
    g_object_get_property (G_OBJECT (mdl), "id", &id);
    g_object_get_property (G_OBJECT (mdl), "building-id", &bid);
    g_object_get_property (G_OBJECT (mdl), "general-count", &cntGeneral);
    g_object_get_property (G_OBJECT (mdl), "business-count", &cntBusiness);

    guint countGeneral = g_value_get_uint (&cntGeneral);
    guint countBusiness = g_value_get_uint (&cntBusiness);

    selfLogInf ("New data [%p] db=%d, build=%d"
        , mdl
        , g_value_get_uint64 (&id)
        , g_value_get_uint64 (&bid)
    );

    // Update data model
    g_object_unref (mainData);
    mainData = NULL;
    mainData = mdl;
    queueSize = g_queue_get_length (queueUnits);

    download_manager_update_cache (mainData);

    // Get main mode
    if (countGeneral && countBusiness) {
        newMode = MAIN_MODE_MENU;
        newViewId = viewMenu;
        newState = state == UI_MainView && queueSize == 0;

    } else {
        newMode = MAIN_MODE_DOORS;
        newViewId = viewMain;
        newState = state == UI_MainMenu;
        if (countBusiness) {
            viewType = MAIN_VIEW_BUSINESSES;
        } else if (countGeneral) {
            viewType = MAIN_VIEW_APARTMENTS;
        }
    }

    selfLogInf ("Old mode %s => new %s%s", view_mode (mainMode), view_mode (newMode), newState ? ", SHOW" : "");
    selfLogInf ("State is %s", view_manager_state_name (state));

    if (newMode != mainMode) { // Mode changed
        mainMode = newMode;
        if (queueSize) {
            selfLogTrc ("Change 1st view");
            (void) g_queue_pop_head (queueUnits);
            view = view_manager_get (newViewId);
            returnIfFailErr (view, "View not initialized!");
            g_queue_push_head (queueUnits, view);
        }
    }

    if (state == UI_Initializing || state == UI_Offline || newState) {
        selfLogTrc ("Show view %s", view_name (newViewId));
        view_manager_show (newViewId, NULL);
    }

    if (newMode == MAIN_MODE_MENU) {
        menu_view_new_data (mdl);
        if (state == UI_MainView) {
            main_view_new_data (mdl, mainViewType);
        } else if (state == UI_UnitView) {
            if (!view_manager_update_unit ()) {
                view_manager_return ();
            }
        }
    } else {
        mainViewType = viewType;
        main_view_new_data (mdl, viewType);
        if (state == UI_UnitView) {
            if (!view_manager_update_unit ()) {
                view_manager_return ();
            }
        }
    }
}

void view_manager_set_new_image (MdlUnit *searchUnit, GdkPixbuf *image) {
    returnIfFailErr (searchUnit, "Unit is NULL");
    returnIfFailErr (image, "Image is NULL");
    returnIfFailErr (MDL_IS_UNIT (searchUnit), "Invalid Unit pointer");
    if (activeView->id != viewMenu) main_view_update_image (searchUnit, image);
    if (activeView->id == viewUnit || activeView->id == viewCallConfirm) {
        unit_view_update_image (searchUnit, image);
    }
    if (activeView->id == viewCallConfirm) call_confirm_view_update_image (searchUnit, image);
}

void view_manager_run (GatewayStatus status) {
    if (status == GW_Online) {
        selfLogInf ("Request UI data");
        MdlDoorbell *mdl = dbus_get_ui_data ();
        selfLogInf ("Got UI data");
        if (mdl) {
            view_manager_set_new_data (mdl);
        }
    } else {
        selfLogTrc ("GW is not online");
        view_manager_show (viewConnecting, NULL);
    }
}

void view_manager_unit (MdlUnit *unit) {
    selectedUserId = unit_get_user (unit);
    selectedRoomId = unit_get_room (unit);
    view_manager_enter (viewUnit, unit);
}


void view_manager_return () {
    ViewManagerStackItem *it;
    guint sz;
    selfLogTrc ("return");

    sz = g_queue_get_length (queueUnits);
    if (!sz) {
        app_quit ();
        return;
    } else {
        if (activeView->id == viewMain) {
            mainViewType = MAIN_VIEW_NONE;
        }
        if (activeView->id == viewUnit) {
            selectedUserId = 0UL;
            selectedRoomId = 0UL;
        }
    }
    it = (ViewManagerStackItem *) g_queue_pop_tail (queueUnits);
    view_manager_show (it->id, NULL);
}

void view_manager_settings () {
    selfLogWrn ("TODO: Enter settings");
}

void view_manager_apartments () {
    selfLogWrn ("Enter apartments");
    view_manager_enter (viewMain, NULL);
    mainViewType = MAIN_VIEW_APARTMENTS;
    main_view_new_data (mainData, mainViewType);
}

void view_manager_businesses () {
    selfLogWrn ("Enter businesses");
    view_manager_enter (viewMain, NULL);
    mainViewType = MAIN_VIEW_BUSINESSES;
    main_view_new_data (mainData, mainViewType);
}

void view_manager_call_confirm (MdlUnit *unit) {
    view_manager_enter (viewCallConfirm, unit);
}

void view_manager_call (gpointer data) {
    // Do not queue current CallConfirm view
    view_manager_show (viewCall, data);
    // Play calling sound
    dbus_play_sound (doorbell_get_call_sound (mainData), FALSE);
}

void view_manager_stop_call () {
    if (state != UI_Call) {
        selfLogErr ("No call");
        return;
    }
    view_manager_return ();
    // Stop calling sound
    dbus_stop_sound (doorbell_get_call_sound (mainData));
    // Reset timeouts
}

void view_manager_on_rfid (gulong code) {
    long tms = g_get_real_time () / 1000;
    if (state == UI_CardAdd) {
        selfLogTrc ("ADD RFID: %X", code);
        dbus_emit_add_card (code, rfidUserId);
    } else if (state == UI_DoorbellSettings) {
        selfLogTrc ("TEST RFID: %X", code);
    } else {
        // Remember last code
        rfidCard = code;
        // Emit open door request
        dbus_emit_open_request (code, tms, "");
    }
}

void view_manager_check_pin (const gchar *message, RfidCheck rc) {
    selfLogTrc ("Check %X pin [%s] pin:%d, isAllowed:%d, isError:%d, state=%s", rfidCard, message, rc.pin, rc.isAllowed, rc.isError, view_manager_get_state_name ());
    if (state == UI_CardPin) {
        pin_view_reset_countdown ();
    } else {
        if (state != UI_Information) {
            selfLogTrc ("Stack active view...");
            view_manager_stack_active_view ();
        }
        selfLogTrc ("Show pin view...");
        view_manager_show (viewPin, NULL);
    }
}

void view_manager_scan_rfid (long userId) {
    selfLogTrc ("Add card for %ld", userId);
    rfidUserId = userId;
    if (state == UI_CardAdd) {
        scan_view_reset_countdown ();
    } else {
        view_manager_enter (viewScan, NULL);
    }
}

void view_manager_scan_finish (long userId) {
    InformationViewData data = { INF_VIEW_MODE_CARD_ADDED, NULL };
    if (state == UI_Information) {
        information_view_update (&data);
    } else {
        view_manager_enter (viewInformation, &data);
    }
}

void view_manager_rfid_exception (const gchar *message, RfidCheck rc) {
    InformationViewData data = { INF_VIEW_MODE_CARD_ERROR, "Error!" };

    if (g_strcmp0 (message, "ROOM_NOT_FOUND") == 0) {
        data.text = "The room is not found.";
    } else if (g_strcmp0 (message, "RFID_CARD_IS_DISABLED") == 0) {
        data.text = "This RFID card is Disabled. Please, try another one.";
    } else if (g_strcmp0 (message, "RFID_CARD_ALREADY_TAKEN") == 0) {
        data.text = "This RFID card already exists. Please, try another one.";
    } else if (g_strcmp0 (message, "TIMEOUT_FOR_ADDING_RFID_CARD") == 0) {
        data.text = "Time to add the card has expired.";
    } else if (g_strcmp0 (message, "ROOM_ALREADY_HAS_THIS_RFID_CARD") == 0) {
        data.text = "Oops, It looks like this RFID card already has been connected to the room.";
    }

    if (state == UI_Information) {
        information_view_update (&data);
    } else if (state == UI_CardPin) {
        pin_view_error ();
    } else {
        view_manager_enter (viewInformation, &data);
    }
}

void view_manager_open_door () {
    InformationViewData data = { INF_VIEW_MODE_DOOR, NULL };
    dbus_play_sound (doorbell_get_door_sound (mainData), TRUE);
    if (state == UI_Information) {
        information_view_update (&data);
    } else if (activeView->type != VIEW_TYPE_BASE) {
        view_manager_show (viewInformation, &data);
    } else {
        view_manager_enter (viewInformation, &data);
    }
}

void view_manager_pin_entered (const gchar *pin) {
    long tms = g_get_real_time () / 1000;
    if (state == UI_CardPin) {
        dbus_emit_open_request (rfidCard, tms, pin);
        // view_manager_return ();
    }
}

void view_manager_lost_connection () {
    InformationViewData data = { INF_VIEW_MODE_NO_CONNECTION, NULL };
    if (hasConnectedKos) {
        data.mode = INF_VIEW_MODE_KOS_ONLY;
    }
    view_manager_reset_stack ();
    view_manager_show (viewInformation, &data);
}

void view_manager_on_gateway_status (GatewayStatus status) {
    selfLogTrc ("GW new status[%d]: %s", status, view_manager_gw_status_name (status));
    switch (status) {
        case GW_Disconnected:
            if (state != UI_Offline) {
                selfLogTrc ("Show lost connection");
                view_manager_lost_connection ();
            }
            break;

        case GW_Online:
            //
            break;

        default: break;
    }
}

void view_manager_on_gateway_property_changed (const char *iface, const char *name) {
    selfLogTrc ("%s new status: %s", iface, name);
}

const gchar *view_manager_get_current_view_name () {
    if (activeView) return view_name (activeView->id);
    return "";
}

AppState view_manager_get_state () {
    return state;
}

const gchar *view_manager_get_state_name () {
    return view_manager_state_name (state);
}

void view_manager_debug () {
    int cnt = 0;
    selfLogTrc ("mnViewType is %s", view_type (mainViewType));
    selfLogTrc ("state      is %s", view_manager_state_name (state));
    selfLogTrc ("activeView is %s", activeView ? view_name (activeView->id) : "empty");
    g_queue_foreach (queueUnits, (GFunc) view_manager_debug_item, &cnt);
    if (cnt == 0) {
        selfLogWrn ("View queue is empty");
    }
}

int view_manager_execute_command (const char *cmd, const char *arg, const char **ans) {
    *ans = "Unknown command";
    RfidCheck rc = { 0 };
    if (g_strcmp0 (cmd, "kos") == 0) {
        hasConnectedKos = g_strcmp0 (arg, "1") == 0 ? TRUE : FALSE;
        if (state == UI_Offline) {
            view_manager_lost_connection ();
        }
        *ans = hasConnectedKos ? "kos:ON" : "kos:OFF";
    }
    if (g_strcmp0 (cmd, "rfid") == 0) {
        if (g_strcmp0 (arg, "ok") == 0) {
            view_manager_scan_finish (0L);
            *ans = "ok";
        } else if (g_strcmp0 (arg, "bad") == 0) {
            view_manager_rfid_exception (NULL, rc);
            *ans = "ok";
        } else {
            *ans = "unknown rfid mode";
        }
    }
    if (g_strcmp0 (cmd, "open") == 0) {
        view_manager_open_door ();
        *ans = "open";
    }
    if (g_strcmp0 (cmd, "queue") == 0) {
        view_manager_debug ();
        *ans = "ok";
    }
    if (g_strcmp0 (cmd, "return") == 0) {
        view_manager_return ();
        *ans = "ok";
    }
    if (g_strcmp0 (cmd, "get") == 0) {
        if (g_strcmp0 (arg, "view") == 0) {
            *ans = view_name (activeView->id);
        } else if (g_strcmp0 (arg, "state") == 0) {
            *ans = (char *)view_manager_state_name (state);
        } else {
            *ans = "unknown variable";
        }
    }
    return 0;
}

static View * view_manager_create_view (ViewId viewId) {
    View *view = g_new0 (View, 1);
    view->id = viewId;
    switch (viewId) {
        case viewMain: main_view_init (app, view); break;
        case viewUnit: unit_view_init (app, view); break;
        case viewMenu: menu_view_init (app, view); break;
        case viewConnecting: connecting_view_init (app, view); break;
        case viewCallConfirm: call_confirm_view_init (app, view); break;
        case viewCall: call_view_init (app, view); break;
        case viewInformation: information_view_init (app, view); break;
        case viewScan: scan_view_init (app, view); break;
        case viewPin: pin_view_init (app, view); break;
        default:
            selfLogWrn ("Invalid view id: %d", viewId);
            g_free (view);
            return NULL;
    }

    selfLogDbg ("Created view %s %s [%s]", view_name (view->id), view->valid ? "OK" : "ERR", gtk_widget_get_name (view->window));
    return view;
}


static void view_manager_enter (ViewId viewId, gpointer data) {
    view_manager_stack_active_view ();
    view_manager_show (viewId, data);
}

static int view_manager_show (ViewId viewId, gpointer data) {
    AppState newState;
    View *view = view_manager_get (viewId);
    returnValIfFailWrn (view, -EINVAL, "Unknown view: %s", view_name (viewId));
    returnValIfFailWrn (view->valid, -EINVAL, "View %s not initialized", view_name (viewId));

    // if (activeView && view->name == activeView->name) return 0;

    selfLogDbg ("Showing %s view: %s [%s] data=%p", view_type (view->type), view_name (view->id), gtk_widget_get_name (view->window), data);

    if (view->type == VIEW_TYPE_POPUP) {
        if (activeView) {
            activeView->blur ();
            gtk_window_set_transient_for (GTK_WINDOW (view->window), GTK_WINDOW (activeView->window));
            gtk_widget_set_parent (GTK_WIDGET (view->window), GTK_WIDGET (activeView->window));
            gtk_window_set_position (GTK_WINDOW (view->window), GTK_WIN_POS_CENTER_ON_PARENT);
        }
    }

    newState = view->show (state, data);

    view_manager_set_state (newState);

    if(view->type != VIEW_TYPE_POPUP) {
        if (activeView && activeView->id != view->id) {
            activeView->hide ();
        }
    }
    activeView = view;
    return 0;
}

static gboolean view_manager_update_unit () {
    GValue val = G_VALUE_INIT;
    GPtrArray *src = NULL;
    const gchar *param;
    gpointer unit;
    guint index = 0;
    gboolean ok;

    // Get required doors source array
    returnValIfFailWrn (mainViewType != MAIN_VIEW_NONE, FALSE, "Unknown unit source [%d]!", view_type (mainViewType));
    param = mainViewType == MAIN_VIEW_BUSINESSES ? "business-doors" : "general-doors";
    g_object_get_property (G_OBJECT (mainData), param, &val);
    src = (GPtrArray *) g_value_get_pointer (&val);

    // Create search model (id only)
    MdlUnit *mdl = unit_new (selectedRoomId, selectedUserId, "");
    // Search the same unit in new data
    ok = g_ptr_array_find_with_equal_func (src, mdl, unit_equal_id, &index);
    unit = g_ptr_array_index (src, index);
    returnValIfFailWrn (ok && unit, FALSE, "Unit {r:%d, u:%d} not found!");

    // Update unit view with new data
    view_manager_show (viewUnit, MDL_UNIT (unit));

    return TRUE;
}

static View *view_manager_get (ViewId viewId) {
    returnValIfFailWrn ((viewId > viewNone && viewId <= VIEW_ID_LAST), NULL, "Wrong viewId=%d", viewId);
    View *view = views[viewId];
    if (!view) {
        view = view_manager_create_view (viewId);
        returnValIfFailWrn (view, NULL, "Wrong viewId=%d", viewId);

        views[viewId] = view;
    }
    return view;
}

static void view_manager_set_state (AppState newState) {
    if (state == newState) return;
    state = newState;
    dbus_emit_state ();
}

static void view_manager_reset_stack () {
    while (g_queue_get_length (queueUnits)) {
        g_free (g_queue_pop_tail (queueUnits));
    }
}

static void view_manager_stack_active_view () {
    ViewManagerStackItem *it;
    if (!activeView) return;
    // Create stack item
    it = g_new0 (ViewManagerStackItem, 1);
    it->id = activeView->id;
    it->type = activeView->type;
    g_queue_push_tail (queueUnits, it);
}

// static void view_manager_pop_stack (ViewId popId) {
//     ViewManagerStackItem *it;
//     guint sz = g_queue_get_length (queueUnits);
//     if (!sz) return;
//     if (popId != viewNone) {
//         it = (ViewManagerStackItem *) g_queue_peek_tail (queueUnits);
//         if (it->id != popId) return;
//     }
//     g_free (g_queue_pop_tail (queueUnits));
// }

static const gchar *view_manager_state_name (AppState state) {
    switch (state) {
        case UI_Initializing:           return "Initialization";
        case UI_Connecting:             return "Connecting";
        case UI_Offline:                return "Offline";
        case UI_OfflineCards:           return "OfflineCards";
        case UI_MainMenu:               return "MainMenu";
        case UI_MainView:               return "MainView";
        case UI_UnitView:               return "UnitView";
        case UI_CallConfirm:            return "CallConfirm";
        case UI_Call:                   return "Call";
        case UI_Information:            return "Information";
        case UI_Settings:               return "Settings";
        case UI_DoorbellSettings:       return "DoorbellSettings";
        case UI_SystemSettings:         return "SystemSettings";
        case UI_SystemActionConfirm:    return "SystemActionConfirm";
        case UI_ServerSettings:         return "ServerSettings";
        case UI_CardAdd:                return "CardAdd";
        case UI_CardPin:                return "CardPin";
        case UI_CardWrong:              return "CardWrong";
        default: break;
    }
    return "Unknown";
}

static const gchar *view_manager_gw_status_name (GatewayStatus state) {
    switch (state) {
        case GW_Initializing:   return  "Initializing";
        case GW_Connecting:     return  "Connecting";
        case GW_Connected:      return  "Connected";
        case GW_Online:         return  "Online";
        case GW_Disconnected:   return  "Disconnected";
        case GW_WaitForModem:   return  "WaitForModem";
        default: break;
    }
    return "Unknown";
}

static void view_manager_debug_item (ViewManagerStackItem *it, int *pCnt) {
    const gchar *t = "Unk";
    switch (it->type) {
        case VIEW_TYPE_NONE:    t = "None"; break;
        case VIEW_TYPE_REGULAR: t = "Reg"; break;
        case VIEW_TYPE_BASE:    t = "Base"; break;
        case VIEW_TYPE_POPUP:   t = "Pop"; break;
    }
    selfLogWrn ("%d> %s [%s]", ++(*pCnt), view_name (it->id), t);
}

