/**
 * @file dbus.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief Service D-bus routines
 * @version 1.0
 * @date 2023-11-21
 *
 *
 *
 */
#include <semaphore.h>

#include "app.h"
#include "bus.h"
#include "sound.h"

// Local function definitions
static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
static int dbus_get_playing_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
static int dbus_play_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError);
static int dbus_stop_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError);
static int dbus_update_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError);

// Local variables
static sd_bus       *bus;

/**
 * @brief Service interface items table
 */
static const sd_bus_vtable vTable[] = {
    SD_BUS_VTABLE_START (BUS_COMMON_FLAGS),
    SD_BUS_METHOD_WITH_NAMES(SOUND_METHOD_PLAY
        , SOUND_METHOD_PLAY_SGN, SD_BUS_PARAM (soundType)
        , SOUND_METHOD_RETURN,   SD_BUS_PARAM (ok)
        , dbus_play_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_METHOD_WITH_NAMES(SOUND_METHOD_STOP
        , SOUND_METHOD_STOP_SGN, SD_BUS_PARAM (soundType)
        , SOUND_METHOD_RETURN,   SD_BUS_PARAM (ok)
        , dbus_stop_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_METHOD_WITH_NAMES(SOUND_METHOD_UPDATE
        , SOUND_UPDATE_SGN,    SD_BUS_PARAM (soundData)
        , SOUND_METHOD_RETURN, SD_BUS_PARAM (ok)
        , dbus_update_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_PROPERTY (SOUND_PROP_STATE,   "y",  dbus_get_state_cb,   0, BUS_COMMON_FLAGS | SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY (SOUND_PROP_PLAYING, "ay", dbus_get_playing_cb, 0, BUS_COMMON_FLAGS | SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_VTABLE_END
};

/**
 * @brief Initialize D-bus UI-service
 *
 * @return int error code
 */
int dbus_init () {
    // Variables
    int r;
    const char* uniqueName;

    // Open main thread bus connection
    r = sd_bus_open_system (&bus);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to open system bus (%d): %s", r, strerror (-r));

    // Add service vtable
    r = sd_bus_add_object_vtable (bus,
                                NULL,
                                DBUS_THIS_PATH,
                                DBUS_THIS_INTERFACE,
                                vTable,
                                NULL);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to issue interface (%d): %s", r, strerror (-r));

    // Take a well-known service name so that clients can find us
    r = sd_bus_request_name (bus, DBUS_THIS_NAME, 0);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to acquire service name (%d): %s", r, strerror (-r));

    r = sd_bus_get_unique_name (bus, &uniqueName);
    if (DBUS_OK (r)) {
        selfLogDbg ("Unique name: %s", uniqueName);
    } else {
        selfLogDbg ("Unique name error (%d): %s", r, strerror (-r));
    }

    selfLogDbg ("D-bus initialization is finished!(%d)", r);
    return r;
}

void dbus_deinit () {
    if (bus)
        sd_bus_unref (bus);
}

int dbus_loop () {
    int r;
    // Process requests
    r = sd_bus_process (bus, NULL);
    returnValIfFailErr (DBUS_OK (r), -r, "Failed to process bus (%d): %s", r, strerror (-r));
    if (r > 0) // we processed a request, try to process another one, right-away
        return EXIT_SUCCESS;

    // Wait for the next request to process
    r = sd_bus_wait (bus, 100000UL);
    returnValIfFailErr (DBUS_OK (r), -r, "Failed to wait on bus: %s", strerror (-r));

    return EXIT_SUCCESS;
}

void dbus_emit_state () {
    returnIfFailErr (bus, "D-bus is not initialized!");

    int r = sd_bus_emit_properties_changed (bus, DBUS_THIS_PATH, DBUS_THIS_INTERFACE, SOUND_PROP_STATE, NULL);
    returnIfFailErr (DBUS_OK (r), "Failed (%d) to emit prop changed signal for `%s`: %s", r, SOUND_PROP_STATE, strerror (-r));

    selfLogDbg ("Emitted changed state: %d", sound_state ());
}

void dbus_emit_playing () {
    returnIfFailErr (bus, "D-bus is not initialized!");

    int pCall = FALSE, pOpen = FALSE;
    sound_playing (&pCall, &pOpen);

    int r = sd_bus_emit_properties_changed (bus, DBUS_THIS_PATH, DBUS_THIS_INTERFACE, SOUND_PROP_PLAYING, NULL);
    returnIfFailErr (DBUS_OK (r), "Failed (%d) to emit prop changed signal for `%s`: %s", r, SOUND_PROP_PLAYING, strerror (-r));

    selfLogDbg ("Emitted changed playing: [%d, %d]", pCall, pOpen);
}


static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    AppState val = sound_state ();
    return sd_bus_message_append (reply, "y", (uint8_t)val);
}

static int dbus_get_playing_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    int pCall = FALSE, pOpen = FALSE;
    sound_playing (&pCall, &pOpen);
    return sd_bus_message_append (reply, "ay", 2, (uint8_t)pCall, (uint8_t)pOpen);
}

static int dbus_play_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    SoundType id = SoundNone;

    // Read method parameter
    r = sd_bus_message_read (m, "y", &id);
    dbusReplyFalseErrorOnFail (r, m, "Read method argument error(%d): %s", r, strerror (-r));

    r = sound_play (id);

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}

static int dbus_stop_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    SoundType id = SoundNone;

    // Read method parameter
    r = sd_bus_message_read (m, "y", &id);
    dbusReplyFalseErrorOnFail (r, m, "Read method argument error(%d): %s", r, strerror (-r));

    r = sound_stop (id);

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}

static int dbus_update_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    int cnt = 0;
    SoundData *data = NULL;
    uint8_t type;
    uint64_t id;
    const char *url;

    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_ARRAY, SOUND_UPDATE_STRUCT);
    dbusReplyFalseErrorOnFail (r, m, "Enter update array error (%d): %s", r, strerror (-r));

    while (sd_bus_message_at_end (m, 0) == 0) {
        r = sd_bus_message_read (m, SOUND_UPDATE_STRUCT, &type, &id, &url);
        if (r >= 0) {
            data = (SoundData *) realloc (data, ((1 + cnt) * sizeof(SoundData)));
            memset (data + cnt, 0, sizeof(SoundData));
            data[cnt].type = (SoundType) type;
            data[cnt].id = id;
            strncpy (data[cnt].url, url, MAX_URL_SIZE);
            cnt++;
        } else {
            selfLogErr ("Read update item error(%d): %s", r, strerror (-r));
        }
    }

    r = sd_bus_message_exit_container (m);
    dbusReplyFalseErrorOnFail (r, m, "Close update array error (%d): %s", r, strerror (-r));

    r = sound_update (data, cnt);

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}