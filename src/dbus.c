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
#include "dbus.h"
#include "sound.h"

// Local function definitions
static const char *dbus_get_error (const sd_bus_error *e, int error);
static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
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
    SD_BUS_METHOD_WITH_NAMES("play"
        , "tb", SD_BUS_PARAM (soundId)
                SD_BUS_PARAM (single)
        , "b",  SD_BUS_PARAM (ok)
        , dbus_play_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_METHOD_WITH_NAMES("stop"
        , "t", SD_BUS_PARAM (soundId)
        , "b", SD_BUS_PARAM (ok)
        , dbus_stop_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_METHOD_WITH_NAMES("update"
        , "tsts", SD_BUS_PARAM (openId)
                  SD_BUS_PARAM (openUrl)
                  SD_BUS_PARAM (callId)
                  SD_BUS_PARAM (callUrl)
        , "b",    SD_BUS_PARAM (ok)
        , dbus_update_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_PROPERTY ("state", "y", dbus_get_state_cb, 0, BUS_COMMON_FLAGS | SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
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

    int r = sd_bus_emit_properties_changed (bus, DBUS_THIS_PATH, DBUS_THIS_INTERFACE, "state", NULL);
    returnIfFailErr (DBUS_OK (r), "Failed (%d) to emit prop changed signal for `%s`: %s", r, UI_PROPERTY_STATE, strerror (-r));

    selfLogDbg ("Emitted changed state: %s", view_manager_get_state_name ());
}

/**
 * @brief Gets dbus error if it's possible
 *
 * @param e Dbus error
 * @param error errno
 * @return const char*
 */
static const char *dbus_get_error (const sd_bus_error *e, int error) {
    if (e) {
        if (sd_bus_error_has_name (e, SD_BUS_ERROR_ACCESS_DENIED))
            return "Access denied";

        if (e->message)
            return e->message;
    }

    return strerror (abs (error));
}


static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    return 0;
}

static int dbus_play_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    int single = 0;
    uint64_t id = 0UL;

    // Read method parameter
    r = sd_bus_message_read (m, "tb", &id, &single);
    if (r >= 0) {
        r = sound_play (id, single);
    } else {
        selfLogErr ("Read method argument error(%d): %s", r, strerror (-r));
        r = 0;
    }

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}

static int dbus_stop_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    uint64_t id = 0UL;

    // Read method parameter
    r = sd_bus_message_read (m, "t", &id);
    if (r >= 0) {
        r = sound_stop (id);
    } else {
        selfLogErr ("Read method argument error(%d): %s", r, strerror (-r));
        r = 0;
    }

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}

static int dbus_update_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError) {
    // Variables
    int r;
    uint64_t idOpen = 0UL, idCall = 0UL;
    const char *urlOpen, *urlCall;

    // Read method parameter
    r = sd_bus_message_read (m, "tsts", &idOpen, &urlOpen, &idCall, &urlCall);
    if (r >= 0) {
        r = sound_update (idOpen, urlOpen, idCall, urlCall);
    } else {
        selfLogErr ("Read method argument error(%d): %s", r, strerror (-r));
        r = 0;
    }

    // Reply bool result
    return sd_bus_reply_method_return(m, "b", r);
}