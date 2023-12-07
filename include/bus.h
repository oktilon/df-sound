#ifndef DBUS_H
#define DBUS_H

#include <systemd/sd-bus.h>

#include "app.h"

// Methods
#define SOUND_METHOD_PLAY               "play"
#define SOUND_METHOD_PLAY_SGN           "y"

#define SOUND_METHOD_STOP               "stop"
#define SOUND_METHOD_STOP_SGN           "y"

#define SOUND_METHOD_UPDATE             "update"
#define SOUND_UPDATE_DATA               "y"
#define SOUND_UPDATE_ITEM               "yts"
#define SOUND_UPDATE_STRUCT             "(" SOUND_UPDATE_ITEM ")"
#define SOUND_UPDATE_SGN                SOUND_UPDATE_DATA "a" SOUND_UPDATE_STRUCT

#define SOUND_METHOD_RETURN             "b"

// Properties
#define SOUND_PROP_STATE                "state"
#define SOUND_PROP_PLAYING              "playing"
#define SOUND_PROP_VOLUME               "volume"

// Subscription
#define DBUS_GW                       "gateway"
#define DBUS_GW_PATH                  DBUS_BASE_PATH DBUS_GW
#define DBUS_GW_NAME                  DBUS_BASE_NAME DBUS_GW
#define DBUS_GW_UI_IFACE              DBUS_GW_NAME ".Ui"
// UI method
#define DBUS_GW_GET_DATA              "getSoundData"
// UI signal
#define DBUS_GW_SIG_DATA              "onSoundData"



int dbus_init ();
void dbus_deinit ();
int dbus_loop ();
int dbus_get_data ();
void dbus_emit_state ();
void dbus_emit_playing ();

#endif // DBUS_H