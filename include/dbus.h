#ifndef DBUS_H
#define DBUS_H
#include <systemd/sd-bus.h>
#include <glib.h>

#include "view_manager.h"
#include "models/mdl_doorbell.h"

GSource *dbus_create_source ();
void dbus_deinit ();
int dbus_loop ();
void dbus_emit_state ();
void dbus_emit_open_request (ulong code, ulong reqId, const gchar *pin);
void dbus_emit_add_card (ulong code, ulong userId);
GatewayStatus dbus_get_gw_status ();
MdlDoorbell *dbus_get_ui_data ();
void dbus_play_sound (guint64 soundId, gboolean single);
void dbus_stop_sound (guint64 soundId);

#endif // DBUS_H