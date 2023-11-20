/**
 * @file dbus.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief Service D-bus routines
 * @version 1.0
 * @date 2023-08-01
 *
 *
 *
 */
#include <gio/gio.h>
#include <semaphore.h>

#include "app.h"
#include "models/mdl_doorbell.h"
#include "dbus.h"

typedef struct SDEventSourceStruct {
    GSource source;
    GPollFD pollfd;
    sd_event *event;
} SDEventSource;

// Local function definitions
static int dbus_init ();
static const char *dbus_get_error (const sd_bus_error *e, int error);
static int dbus_get_current_view_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
static int dbus_get_state_name_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError);
static int dbus_on_ui_signal_handler (sd_bus_message *m, void *_data, sd_bus_error *retError);
static int dbus_on_ui_data_handler (sd_bus_message *m, sd_bus_error *retError);
static int dbus_on_rfid_handler (sd_bus_message *m, void *_data, sd_bus_error *retError);
static int dbus_on_gateway_property_status (sd_bus_message *m, uint8_t *ret);
static int dbus_on_gateway_property_read (sd_bus_message *m, const char *iface);
static int dbus_on_gateway_property_changed (sd_bus_message *m, void *_data, sd_bus_error *retError);
static int dbus_execute_command_cb (sd_bus_message *m, void *userdata, sd_bus_error *retError);
static gboolean dbus_source_prepare(GSource *source, gint *timeout_);
static gboolean dbus_source_check(GSource *source);
static gboolean dbus_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data);
static void dbus_source_finalize(GSource *source);

// Local variables
static sd_bus       *bus;
static GSourceFuncs  sourceFuncs = {
    .prepare = dbus_source_prepare,
    .check = dbus_source_check,
    .dispatch = dbus_source_dispatch,
    .finalize = dbus_source_finalize
};

/**
 * @brief Service interface items table
 */
static const sd_bus_vtable vtableInterface[] = {
    SD_BUS_VTABLE_START (BUS_COMMON_FLAGS),
    SD_BUS_METHOD_WITH_NAMES(UI_METHOD_CMD
        , "ss", SD_BUS_PARAM (command)
                SD_BUS_PARAM (argument)
        , "s", SD_BUS_PARAM (answer)
        , dbus_execute_command_cb
        , BUS_COMMON_FLAGS
    ),
    SD_BUS_PROPERTY (UI_PROPERTY_CURRENT_VIEW, "s", dbus_get_current_view_cb, 0, BUS_COMMON_FLAGS),
    SD_BUS_PROPERTY (UI_PROPERTY_STATE, "y", dbus_get_state_cb, 0, BUS_COMMON_FLAGS | SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY (UI_PROPERTY_STATE_NAME, "s", dbus_get_state_name_cb, 0, BUS_COMMON_FLAGS),
    SD_BUS_SIGNAL_WITH_NAMES (UI_SIGNAL_OPEN_DOOR
        , "sss", SD_BUS_PARAM (code)
                 SD_BUS_PARAM (pin)
                 SD_BUS_PARAM (req_id)
        , BUS_COMMON_FLAGS),
    SD_BUS_VTABLE_END
};

/**
 * @brief Initialize D-bus UI-service
 *
 * @return int error code
 */
static int dbus_init () {
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
                                vtableInterface,
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


    // Subscribe Gateway : Ui interface signals
    r = sd_bus_match_signal (bus, NULL, NULL, DBUS_GW_PATH, DBUS_GW_IFACE_UI, NULL, dbus_on_ui_signal_handler, NULL);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to subscribe %s from %s (%d): %s", GW_SIG_UI_DATA, DBUS_GW_IFACE_UI, r, strerror (-r));

    // Subscribe GW : PropertiesChanged
    r = sd_bus_match_signal (bus, NULL, NULL, DBUS_GW_PATH, DBUS_PROPERTIES_INTERFACE, DBUS_PROPERTIES_CHANGED, dbus_on_gateway_property_changed, NULL);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to subscribe %s from %s (%d): %s", DBUS_PROPERTIES_CHANGED, DBUS_PROPERTIES_INTERFACE, r, strerror (-r));

    // Subscribe Rfid : rfid_read
    r = sd_bus_match_signal (bus, NULL, NULL, DBUS_RFID_PATH, DBUS_RFID_IFACE, RFID_SIGNAL_READ, dbus_on_rfid_handler, NULL);
    returnValIfFailErr (DBUS_OK (r), r, "Failed to subscribe %s from %s (%d): %s", RFID_SIGNAL_READ, DBUS_RFID_IFACE, r, strerror (-r));


    selfLogDbg ("D-bus initialization is finished!(%d)", r);
    return r;
}

void dbus_deinit () {
    if (bus)
        sd_bus_unref (bus);
}

GSource *dbus_create_source () {
    int r;

    SDEventSource *source = (SDEventSource *) g_source_new (&sourceFuncs, sizeof (SDEventSource));

    // Create default SD event
    r = sd_event_default (&source->event);
    if (r < 0) {
        selfLogErr ("Failed on sd_event_new(%d): %s", r, strerror (-r));
        goto __err;
    }

    // Init D-bus
    r = dbus_init ();
    if (r < 0) {
        goto __err;
    }

    // Attaching event to dbus
    r = sd_bus_attach_event (bus, source->event, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0) {
        selfLogErr ("Failed on sd_bus_attach_event(%d): %s", r, strerror (-r));
        goto __err;
    }

    // Initializing source
    source->pollfd.fd = (gint) sd_event_get_fd (source->event);
    source->pollfd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;

    // Adding source to poll
    g_source_add_poll ((GSource *) source, &source->pollfd);

    return (GSource *) source;

__err:
    sd_event_unref (source->event);
    g_source_destroy ((GSource *) source);
    g_source_unref ((GSource *) source);
    return NULL;
}

int dbus_loop () {
    int r;
    // Process requests
    r = sd_bus_process (bus, NULL);
    returnValIfFailErr (DBUS_OK (r), -r, "Failed to process bus (%d): %s", r, strerror (-r));
    if (r > 0) // we processed a request, try to process another one, right-away
        return EXIT_SUCCESS;

    // Wait for the next request to process
    // r = sd_bus_wait (bus, 1UL);
    // returnValIfFailErr (DBUS_OK (r), -r, "Failed to wait on bus: %s", strerror (-r));

    return EXIT_SUCCESS;
}

void dbus_emit_state () {
    returnIfFailErr (bus, "D-bus is not initialized!");

    int r = sd_bus_emit_properties_changed (bus, DBUS_THIS_PATH, DBUS_THIS_INTERFACE, UI_PROPERTY_STATE, NULL);
    returnIfFailErr (DBUS_OK (r), "Failed (%d) to emit prop changed signal for `%s`: %s", r, UI_PROPERTY_STATE, strerror (-r));

    selfLogDbg ("Emitted changed state: %s", view_manager_get_state_name ());
}

void dbus_emit_open_request (ulong code, ulong reqId, const gchar *pin) {
    returnIfFailErr (bus, "D-bus is not initialized!");
    static gchar sCode[25] = {0};
    static gchar sReqId[25] = {0};

    // selfLogTrc ("got rfid=%ld [0x%X]", code, code);

    snprintf (sCode, 24, "%ld", code);
    snprintf (sReqId, 24, "%ld", reqId);

    int r = sd_bus_emit_signal (bus
        , DBUS_THIS_PATH
        , DBUS_THIS_INTERFACE
        , UI_SIGNAL_OPEN_DOOR
        , "sss"
        , sCode
        , pin
        , sReqId);
    returnIfFailErr (DBUS_OK (r), "Failed to emit %s signal(%d): %s", UI_SIGNAL_OPEN_DOOR, r, strerror (-r));

    selfLogTrc ("Emitted %s (\"%ld\", \"%s\", \"%ld\")", UI_SIGNAL_OPEN_DOOR, code, pin, reqId);
}

void dbus_emit_add_card (ulong code, ulong userId) {
    returnIfFailErr (bus, "D-bus is not initialized!");
    static gchar sCode[25] = {0};

    // selfLogTrc ("got rfid=%ld [0x%X]", code, code);

    snprintf (sCode, 24, "%ld", code);

    int r = sd_bus_emit_signal (bus
        , DBUS_THIS_PATH
        , DBUS_THIS_INTERFACE
        , UI_SIGNAL_ADD_CARD
        , "is"
        , userId
        , sCode);
    returnIfFailErr (DBUS_OK (r), "Failed to emit %s signal(%d): %s", UI_SIGNAL_ADD_CARD, r, strerror (-r));

    selfLogTrc ("Emitted %s (%ld, 0x%08X=%ld)", UI_SIGNAL_ADD_CARD, userId, code, code);
}

GatewayStatus dbus_get_gw_status () {
    // Variables
    GatewayStatus ret = GW_Initializing;
    int r;
    uint8_t u = 0;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message *ans = NULL;

    // Get GW:status property (Connection interface)
    r = sd_bus_get_property (bus
        , DBUS_GW_NAME
        , DBUS_GW_PATH
        , DBUS_GW_IFACE_CONN
        , GW_PROP_STATUS
        , &err
        , &ans
        , "y");
    returnValIfFailErr (DBUS_OK (r), ret, "Get GW status error (%d): %s", r, dbus_get_error (&err, r));

    // Read status value (byte)
    r = sd_bus_message_read_basic (ans, 'y', &u);
    returnValIfFailErr (DBUS_OK (r), ret, "Read GW status error (%d): %s", r, strerror (-r));

    ret = (GatewayStatus)u;

    return ret;
}

MdlDoorbell *dbus_get_ui_data () {
    // Variables
    MdlDoorbell *ret = NULL;
    int r;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message *ans;

    selfLogDbg ("Try to get UI data...");
    // Call GW:getUiData method (Ui interface)
    r = sd_bus_call_method (bus
        , DBUS_GW_NAME
        , DBUS_GW_PATH
        , DBUS_GW_IFACE_UI
        , GW_METHOD_GET_UI_DATA
        , &err
        , &ans
        , ""
    );
    returnValIfFailErr (DBUS_OK (r), ret, "Get GW UI data error (%d): %s", r, dbus_get_error (&err, r));

    // Read and parse data
    ret = doorbell_new (ans);
    returnValIfFailErr (ret, ret, "Read GW UI data error: %m");

    return ret;
}

void dbus_play_sound (guint64 soundId, gboolean single) {
    // Variables
    int r;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message *ans = NULL;

    selfLogTrc ("Try to play %ssound %d ...", single ? "single " : "", soundId);

    // Call sound:play method
    r = sd_bus_call_method (bus
        , DBUS_SOUND_NAME
        , DBUS_SOUND_PATH
        , DBUS_SOUND_IFACE
        , SOUND_METHOD_PLAY
        , &err
        , &ans
        , "tb"
        , soundId
        , single
    );
    if (r < 0) selfLogErr ("Call sound:play error (%d): %s", r, dbus_get_error (&err, r));
    sd_bus_error_free (&err);
    if (ans) sd_bus_message_unref (ans);
}

void dbus_stop_sound (guint64 soundId) {
    // Variables
    int r;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message *ans = NULL;

    selfLogTrc ("Try to stop sound %d ...", soundId);

    // Call sound:play method
    r = sd_bus_call_method (bus
        , DBUS_SOUND_NAME
        , DBUS_SOUND_PATH
        , DBUS_SOUND_IFACE
        , SOUND_METHOD_STOP
        , &err
        , &ans
        , "t"
        , soundId
    );
    if (r < 0) selfLogErr ("Call sound:stop error (%d): %s", r, dbus_get_error (&err, r));
    sd_bus_error_free (&err);
    if (ans) sd_bus_message_unref (ans);
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

static int dbus_get_current_view_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    const char *val = view_manager_get_current_view_name ();
    return sd_bus_message_append (reply, "s", val);
}

static int dbus_get_state_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    AppState val = view_manager_get_state ();
    return sd_bus_message_append (reply, "y", (uint8_t)val);
}

static int dbus_get_state_name_cb (sd_bus *b, const char *p, const char *i, const char *name, sd_bus_message *reply, void *_data, sd_bus_error *retError) {
    const char *val = view_manager_get_state_name ();
    return sd_bus_message_append (reply, "s", val);
}

static int dbus_on_ui_signal_handler (sd_bus_message *m, void *_data, sd_bus_error *retError) {
    int r = 0;
    long userId;
    RfidCheck rc = {0};
    const gchar *message;
    const gchar *line;
    const gchar *member = sd_bus_message_get_member (m);

    if (g_strcmp0 (member, GW_SIG_UI_DATA) == 0) {
        return dbus_on_ui_data_handler (m, retError);
    } else if (g_strcmp0 (member, GW_SIG_ADD_CARD) == 0) {
        r = sd_bus_message_read (m, "x", &userId);
        dbusErrorOnFail (r, r, "Read error", "AddCard argument read error", retError);
        view_manager_scan_rfid (userId);
    } else if (g_strcmp0 (member, GW_SIG_RFID_EXCEPTION) == 0) {
        r = sd_bus_message_read (m, "sy", &message, &(rc.flags));
        dbusErrorOnFail (r, r, "Read error", "RfidException arguments read error", retError);
        view_manager_rfid_exception (message, rc);
    } else if (g_strcmp0 (member, GW_SIG_RFID_CHECK) == 0) {
        r = sd_bus_message_read (m, "sy", &message, &(rc.flags));
        dbusErrorOnFail (r, r, "Read error", "RfidCheck arguments read error", retError);
        view_manager_check_pin (message, rc);
    } else if (g_strcmp0 (member, GW_SIG_RFID_ADDED) == 0) {
        r = sd_bus_message_read (m, "x", &userId);
        dbusErrorOnFail (r, r, "Read error", "CardAdded argument read error", retError);
        view_manager_scan_finish (userId);
    } else if (g_strcmp0 (member, GW_SIG_OPEN_DOOR) == 0) {
        // Relay data from signal isn't required
        view_manager_open_door ();
    } else if (g_strcmp0 (member, GW_SIG_SS_DATA) == 0) {
        r = sd_bus_message_read (m, "ss", &message, &line);
        dbusErrorOnFail (r, r, "Read error", "SSData arguments read error", retError);
        // Just ignore this signal
        (void) message;
        (void) line;
    } else {
        selfLogWrn ("Ui signal %s not expected!", member);
    }
    return r;
}

static int dbus_on_ui_data_handler (sd_bus_message *m, sd_bus_error *retError) {
    // Create model by parsing message
    MdlDoorbell *mdl = doorbell_new (m);
    if (!mdl) {
        sd_bus_error_set_const (retError, "Model error", "Model create/parse error");
        return -1;
    }
    // Call callback
    view_manager_set_new_data (mdl);
    return 0;
}

static int dbus_on_rfid_handler (sd_bus_message *m, void *_data, sd_bus_error *retError) {
    int r, cnt = 0;
    uint8_t byte;
    uint32_t len;
    uint64_t code = 0;

    r = sd_bus_message_read_basic(m, 'i', &len);
    dbusOnFailErr (r, "Rfid read length error(%d) %s", r, strerror (-r));

    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_ARRAY, "y");
    dbusOnFailErr (r, "Rfid byte array open error(%d) %s", r, strerror (-r));

    while (sd_bus_message_at_end (m, FALSE) == 0) {
        r = sd_bus_message_read_basic(m, 'y', &byte);
        code <<= 8;
        if (r < 0) {
            byte = 0;
            selfLogWrn ("Read byte #%d error(%d): %s", cnt, r, strerror (-r));
        }
        code |= byte;
        cnt++;
    }

    r = sd_bus_message_exit_container (m);
    dbusOnFailErr (r, "Rfid byte array close error(%d) %s", r, strerror (-r));

    view_manager_on_rfid (code);
    return 0;
}

static int dbus_on_gateway_property_status (sd_bus_message *m, uint8_t *ret) {
    int r;

    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_VARIANT, "y");
    dbusOnFailErr (r, "Gateway status variant open error(%d) %s", r, strerror (-r));

    r = sd_bus_message_read_basic(m, 'y', ret);
    dbusOnFailErr (r, "Gateway status variant read byte error(%d) %s", r, strerror (-r));

    r = sd_bus_message_exit_container (m);
    dbusOnFailErr (r, "Gateway status variant close error(%d) %s", r, strerror (-r));

    return r;
}

static int dbus_on_gateway_property_read (sd_bus_message *m, const char *iface) {
    int r;
    uint8_t status = 0;
    const char *name;
    // Read Properties without values
    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_DICT_ENTRY, "sv");
    dbusOnFailErr (r, "PropertiesChanged open dict error(%d) %s", r, strerror (-r));

    while (sd_bus_message_at_end (m, FALSE) == 0) {
        r = sd_bus_message_read_basic(m, 's', &name);
        dbusOnFailErr (r, "PropertiesChanged read prop name error(%d) %s", r, strerror (-r));
        if (g_strcmp0 (name, "status") == 0) {
            r = dbus_on_gateway_property_status (m, &status);
            if (r < 0) return r;
            view_manager_on_gateway_status ((GatewayStatus) status);
        } else {
            // Ignore other properties
            sd_bus_message_skip (m, "v");
        }
    }

    r = sd_bus_message_exit_container (m);
    dbusOnFailErr (r, "PropertiesChanged close dict error(%d) %s", r, strerror (-r));
    return r;
}

static int dbus_on_gateway_property_changed (sd_bus_message *m, void *_data, sd_bus_error *retError) {
    // Variables
    int r;
    const char *iface = NULL;
    const char *value = NULL;
    const char *name = sd_bus_message_get_member (m);
    // sa{sv}as = Interface, [{Parameter, Value}], [Parameter]
    // Get sender interface name [s]
    r = sd_bus_message_read_basic(m, 's', &iface);
    dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged read interface error", retError);

    // Read Properties with values [a{sv}]
    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_ARRAY, "{sv}");
    dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged open dict array error", retError);

    while (sd_bus_message_at_end (m, FALSE) == 0) {
        r = dbus_on_gateway_property_read (m, iface);
        if (r < 0) break;
    }

    r = sd_bus_message_exit_container (m);
    dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged close dict array error", retError);

    // Read Properties without values [as]
    r = sd_bus_message_enter_container (m, SD_BUS_TYPE_ARRAY, "s");
    dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged open names array error", retError);

    while (sd_bus_message_at_end (m, FALSE) == 0) {
        r = sd_bus_message_read_basic(m, 's', &value);
        dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged read prop name error", retError);
        view_manager_on_gateway_property_changed (iface, name);
    }

    r = sd_bus_message_exit_container (m);
    dbusErrorOnFail (r, -1, "Parse error", "PropertiesChanged close names array error", retError);

    // Call callback
    return 0;
}

static int dbus_execute_command_cb (sd_bus_message *m, void *_data, sd_bus_error *retError) {
    // Variables
    char *cmd = NULL;
    char *arg = NULL;
    const char *ans = NULL;
    // Read method parameter
    int r = sd_bus_message_read(m, "ss", &cmd, &arg);
    dbusReplyErrorOnFail (r, "Parser error", "Can't get argument value", m);

    r = view_manager_execute_command (cmd, arg, &ans);
    dbusReplyErrorOnFail (r, "Execute error", ans ? ans : "Unknown error", m);
    // Reply bool result
    r = sd_bus_reply_method_return(m, "s", ans ? ans : "ok");
    return r;
}

static gboolean dbus_source_prepare (GSource *source, gint *timeout_) {
    return sd_event_prepare (((SDEventSource *) source)->event) > 0;
}

static gboolean dbus_source_check (GSource *source) {
    return sd_event_wait (((SDEventSource *) source)->event, 0) > 0;
}

static gboolean dbus_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data) {
    return sd_event_dispatch (((SDEventSource *) source)->event) > 0;
}

static void dbus_source_finalize (GSource *source) {
    sd_event_unref (((SDEventSource *) source)->event);
}
