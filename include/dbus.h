#ifndef DBUS_H
#define DBUS_H

#include <systemd/sd-bus.h>

#include "app.h"

int dbus_init ();
void dbus_deinit ();
int dbus_loop ();
void dbus_emit_state ();

#endif // DBUS_H