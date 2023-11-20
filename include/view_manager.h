#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "app.h"
#include "view.h"
#include "models/mdl_doorbell.h"
#include "view_history.h"

#include "views/view_pin.h"
#include "views/view_menu.h"
#include "views/view_main.h"
#include "views/view_unit.h"
#include "views/view_call.h"
#include "views/view_scan.h"
#include "views/view_connecting.h"
#include "views/view_information.h"
#include "views/view_call_confirm.h"


// Init
void view_manager_init (GtkApplication *app);

// Data update handlers
void view_manager_set_new_data (MdlDoorbell *mdl);
void view_manager_set_new_image (MdlUnit *unit, GdkPixbuf *image);

// View switching
void view_manager_run (GatewayStatus status);
void view_manager_unit (MdlUnit *unit);
void view_manager_return ();
void view_manager_settings ();
void view_manager_apartments ();
void view_manager_businesses ();

// Call processing
void view_manager_call_confirm (MdlUnit *unit);
void view_manager_call (gpointer data);
void view_manager_stop_call ();

// Rfid processing
void view_manager_on_rfid (ulong code);
void view_manager_check_pin (const gchar *message, RfidCheck rc);
void view_manager_scan_rfid (long userId);
void view_manager_scan_finish (long userId);
void view_manager_rfid_exception (const gchar *message, RfidCheck rc);
void view_manager_open_door ();
void view_manager_pin_entered (const gchar *pin);

// Information views
void view_manager_lost_connection ();

// Update handlers
void view_manager_on_gateway_status (GatewayStatus status);
void view_manager_on_gateway_property_changed (const char *iface, const char *name);

// Query methods
const gchar *view_manager_get_current_view_name ();
AppState view_manager_get_state ();
const gchar *view_manager_get_state_name ();

// Debug methods
void view_manager_debug ();
int view_manager_execute_command (const char *cmd, const char *arg, const char **ans);

#endif // VIEW_MANAGER_H