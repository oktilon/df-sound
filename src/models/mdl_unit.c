/**
 * @file mdl_unit.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief Unit model inherited of GObject
 * @version 0.1
 * @date 2023-08-17
 *
 *
 *
 */
#include <ctype.h>

#include "models/mdl_unit.h"
#include "app.h"

/**
 * @brief Door data class
 */
typedef struct _MdlUnitPrivate {
    uint64_t roomId;
    uint64_t userId;
    uint64_t number;
    uint64_t sequentialNumber;
    uint64_t callTimeOut;
    UnitType type;
    gchar *imgUrl;
    gchar *header;
    gchar *text;
    mdlUnitConfig unitConfig;
    weekDays daysOfWeek;
    /**
     * @brief Time of a day in seconds
     * (00:00:00-23:59:59) = (0 - 86400)
     */
    uint32_t startTime;
    /**
     * @brief time of a day in seconds
     * (00:00:00-23:59:59) = (0 - 86400)
     */
    uint32_t endTime;
    GPtrArray *members;
} MdlUnitPrivate;

/**
 * @brief Doorbell data parsed for services
 */
struct _MdlUnit {
    GObject parent_instance;
};

enum {
    PROP_0,
    PROP_ROOM,
    PROP_USER,
    PROP_NUMBER,
    PROP_SEQUENCE,
    PROP_TIMEOUT,
    PROP_TYPE,
    PROP_IMAGE,
    PROP_HEADER,
    PROP_TEXT,
    PROP_DISABLED,
    PROP_DISABLED_SCHEDULE,
    PROP_DISABLE_SEQUENCE,
    PROP_WEEKDAYS,
    PROP_START_TIME,
    PROP_END_TIME,
    PROP_MEMBERS,

    N_PROPERTIES
};

static GParamSpec   *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (MdlUnit, unit, G_TYPE_OBJECT)

#define GET_PRIVATE(d) ((MdlUnitPrivate *) unit_get_instance_private ((MdlUnit*)d))

static void unit_init (MdlUnit *mdl) {
    MdlUnitPrivate *priv = GET_PRIVATE (mdl);
    priv->members = g_ptr_array_new_full (0, g_object_unref);
    // selfLogDbg ("Init unit %p", mdl);
}

static void unit_dispose (GObject *object) {
    MdlUnitPrivate *priv = GET_PRIVATE (object);
    // selfLogTrc ("Dispose unit %s [%p]", priv->header, priv->header);

    // free doors
    if (priv->members) {
        g_ptr_array_free (priv->members, TRUE);
        priv->members = NULL;
    }

    G_OBJECT_CLASS (unit_parent_class)->finalize (object);
}

static void unit_finalize (GObject *object) {
    MdlUnitPrivate *priv = GET_PRIVATE (object);
    // selfLogTrc ("Finalize unit %s [%p]", priv->header, priv->header);

    if (priv->imgUrl) {
        g_free (priv->imgUrl);
    }
    if (priv->header) {
        g_free (priv->header);
    }
    if (priv->text) {
        g_free (priv->text);
    }

    G_OBJECT_CLASS (unit_parent_class)->finalize (object);
}

static void unit_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    MdlUnitPrivate *priv;

    g_return_if_fail (MDL_IS_UNIT (object));
    priv = GET_PRIVATE (object);

    switch (prop_id) {
        case PROP_ROOM:
            g_value_set_uint64 (value, priv->roomId);
            break;

        case PROP_USER:
            g_value_set_uint64 (value, priv->userId);
            break;

        case PROP_NUMBER:
            g_value_set_uint64 (value, priv->number);
            break;

        case PROP_SEQUENCE:
            g_value_set_uint64 (value, priv->sequentialNumber);
            break;

        case PROP_TIMEOUT:
            g_value_set_uint64 (value, priv->callTimeOut);
            break;

        case PROP_TYPE:
            g_value_set_uchar (value, priv->type);
            break;

        case PROP_IMAGE:
            g_value_take_string (value, priv->imgUrl);
            break;

        case PROP_HEADER:
            g_value_take_string (value, priv->header);
            break;

        case PROP_TEXT:
            g_value_take_string (value, priv->text);
            break;

        case PROP_DISABLED:
            g_value_set_boolean (value, (gboolean)priv->unitConfig.disabled);
            break;

        case PROP_DISABLED_SCHEDULE:
            g_value_set_boolean (value, (gboolean)priv->unitConfig.disabledSchedule);
            break;

        case PROP_DISABLE_SEQUENCE:
            g_value_set_boolean (value, (gboolean)priv->unitConfig.disableSequence);
            break;

        case PROP_WEEKDAYS:
            g_value_set_uchar (value, priv->daysOfWeek.fullWeek);
            break;

        case PROP_START_TIME:
            g_value_set_uint (value, priv->startTime);
            break;

        case PROP_END_TIME:
            g_value_set_uint (value, priv->endTime);
            break;

        case PROP_MEMBERS:
            g_value_set_object (value, priv->members);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void unit_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    g_return_if_fail (MDL_IS_UNIT (object));

    switch (prop_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void unit_class_init (MdlUnitClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = unit_dispose;
    object_class->finalize = unit_finalize;
    object_class->set_property = unit_set_property;
    object_class->get_property = unit_get_property;

    properties[PROP_ROOM] = g_param_spec_uint64 ("room", "roomId", "Room Id", 0, UINT64_MAX, 0, G_PARAM_READABLE);
    properties[PROP_USER] = g_param_spec_uint64 ("user", "userId", "User Id", 0, UINT64_MAX, 0, G_PARAM_READABLE);
    properties[PROP_NUMBER] = g_param_spec_uint64 ("number", "number", "Room number", 0, UINT64_MAX, 0, G_PARAM_READABLE);
    properties[PROP_SEQUENCE] = g_param_spec_uint64 ("sequence", "sequentialNumber", "Sequential number", 0, UINT64_MAX, 0, G_PARAM_READABLE);
    properties[PROP_TIMEOUT] = g_param_spec_uint64 ("timeout", "callTimeOut", "Call timeout", 0, UINT64_MAX, 0, G_PARAM_READABLE);

    properties[PROP_TYPE] = g_param_spec_uint ("type", "type", "Unit type", 0, UINT32_MAX, 0, G_PARAM_READABLE);

    properties[PROP_IMAGE] = g_param_spec_string ("image", "imageUrl", "Unit image URL", "", G_PARAM_READABLE);
    properties[PROP_HEADER] = g_param_spec_string ("header", "header", "Unit header", "", G_PARAM_READABLE);
    properties[PROP_TEXT] = g_param_spec_string ("text", "text", "Unit text", "", G_PARAM_READABLE);

    properties[PROP_DISABLED] = g_param_spec_boolean ("disabled", "disabled", "Disabled", FALSE, G_PARAM_READABLE);
    properties[PROP_DISABLED_SCHEDULE] = g_param_spec_boolean ("disabled-schedule", "disabledSchedule", "Disabled schedule", FALSE, G_PARAM_READABLE);
    properties[PROP_DISABLE_SEQUENCE] = g_param_spec_boolean ("disabled-sequence", "disableSequence", "Disable sequence", FALSE, G_PARAM_READABLE);

    properties[PROP_WEEKDAYS] = g_param_spec_uint ("weekdays", "daysOfWeek", "Days of week", 0, UINT32_MAX, 0, G_PARAM_READABLE);
    properties[PROP_START_TIME] = g_param_spec_uint ("start-time", "startTime", "Start time", 0, UINT32_MAX, 0, G_PARAM_READABLE);
    properties[PROP_END_TIME] = g_param_spec_uint ("end-time", "endTime", "End time", 0, UINT32_MAX, 0, G_PARAM_READABLE);

    properties[PROP_MEMBERS] = g_param_spec_pointer ("members", "members", "Unit members", G_PARAM_READABLE);


    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* Public API */

MdlUnit *unit_new (uint64_t room, uint64_t user, const gchar *name) {
    // Variable
    MdlUnitPrivate *priv;
    MdlUnit *mdl = (MdlUnit *) g_object_new (MDL_TYPE_UNIT, NULL);

    if (!(MDL_IS_UNIT (mdl))) {
        selfLogErr ("Create model error: %m");
        return NULL;
    }
    priv = GET_PRIVATE (mdl);
    priv->roomId = room;
    priv->userId = user;
    priv->header = g_strdup (name);

    return mdl;
}

MdlUnit *unit_parse_new (sd_bus_message *msg, int isMember) {
    // Variable
    int r;
    const char *signature = UNIT_DATA_SIGNATURE;
    MdlUnitPrivate *priv;
    MdlUnit *unit;
    MdlUnit *mdl = (MdlUnit *) g_object_new (MDL_TYPE_UNIT, NULL);
    const gchar *tmpHeader;
    const gchar *tmpText;
    const gchar *tmpUrl;
    GString *strHeader;
    GString *strText;
    gboolean hasDesc = FALSE;

    // Check model
    returnValIfFailErr (MDL_IS_UNIT (mdl), NULL, "Create model error: %m");
    priv = GET_PRIVATE (mdl);

    // Prepare Unit signature
    if (isMember) {
        signature = UNIT_BASE_SIGNATURE;
    }

    // Create business doors array container
    r = sd_bus_message_enter_container (msg, SD_BUS_TYPE_STRUCT, signature);
    // On error log it and return
    returnValIfFailErr (DBUS_OK (r), NULL, "Enter unit struct container error (%d): %s", r, strerror (-r));

    uint8_t uType = 0;

    // Read doorbell self data
    r = sd_bus_message_read (msg, UNIT_BASE_SIGNATURE
        , &(priv->roomId)
        , &(priv->userId)
        , &(priv->number)
        , &(priv->sequentialNumber)
        , &(priv->callTimeOut)
        , &(uType)
        , &(tmpUrl)
        , &(tmpHeader)
        , &(tmpText)
        , &(priv->unitConfig.config)
        , &(priv->daysOfWeek.fullWeek)
        , &(priv->startTime)
        , &(priv->endTime)
    );
    if (r < 0) {
        selfLogErr ("Read message error (%d): %s", r, strerror (-r));
    }

    priv->type = (UnitType)uType;

    hasDesc = tmpText && strlen (tmpText) ? TRUE : FALSE;

    // Purge texts
    strHeader = g_string_new (tmpHeader);
    strText = g_string_new (tmpText);
    g_string_replace (strHeader, "\n", " ", 0);
    g_string_replace (strText, "\n", " ", 0);
    priv->header = g_string_free (strHeader, FALSE);
    priv->text = g_string_free (strText, FALSE);
    priv->imgUrl = g_strdup (tmpUrl);

    if (!isMember) {
        // Enter members array container
        r = sd_bus_message_enter_container (msg, SD_BUS_TYPE_ARRAY, UNIT_MEMBER_STRUCT_SIGNATURE);
        dbusReturnOnFailErr (r, NULL, "Enter unit members array error (%d): %s", r, strerror (-r));

        while (sd_bus_message_at_end (msg, FALSE) == 0) {
            unit = unit_parse_new (msg, TRUE);
            if (unit) {
                g_ptr_array_add (priv->members, unit);
            }
        }

        // Exit members array container
        r = sd_bus_message_exit_container (msg);
        dbusReturnOnFailErr (r, NULL, "Exit unit members array error (%d): %s", r, strerror (-r));

    }

    // Exit members array container
    r = sd_bus_message_exit_container (msg);
    dbusReturnOnFailErr (r, NULL, "Close business doors array error (%d): %s", r, strerror (-r));

    if (!strlen (priv->imgUrl)) {
        priv->unitConfig.imageLoaded = 1;
    }

    const gchar *url = priv->imgUrl ? priv->imgUrl : NULL;
    if (strlen (url) == 0) url = NULL;
    selfLogDbg ("%s%s r% 7u u% 7u \"%s\"%s%s%s %s"
        , isMember ? "--" : "* "
        , priv->userId > 0 ? "U" : "R"
        , priv->roomId
        , priv->userId
        , priv->header
        , hasDesc ? " (" : ""
        , priv->text
        , hasDesc ? ")" : ""
        , url ? "<IMG>" : "");
    return mdl;
}

void unit_image_loaded (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_if_fail (MDL_IS_UNIT (mdl));
    // Get private
    priv = GET_PRIVATE (mdl);
    if (strlen (priv->imgUrl)) {
        priv->unitConfig.imageLoaded = 1;
    }
}

gboolean unit_is_image_loaded (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), FALSE);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->unitConfig.imageLoaded ? TRUE : FALSE;
}

guint64 unit_get_room (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), 0);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->roomId;
}
guint64 unit_get_user (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), 0);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->userId;
}


const gchar *unit_get_header (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), "");
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->header;
}

const gchar *unit_get_text (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), "");
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->text;
}

const gchar *unit_get_image_url (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), "");
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->imgUrl;
}

gboolean unit_has_members (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), FALSE);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return ui_ptr_array_length (priv->members) > 0 ? TRUE : FALSE;
}

GPtrArray * unit_get_members (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), FALSE);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return priv->members;
}

UnitType unit_get_unit_type (MdlUnit *mdl) {
    // Variable
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), FALSE);
    // Get private
    priv = GET_PRIVATE (mdl);
    // Return value
    return (UnitType)priv->type;
}

gboolean unit_contains_text (gpointer *unit, const gchar *text) {
    // Variables
    MdlUnitPrivate *priv;
    gboolean ok;
    GRegex *reg;
    GError *err = NULL;
    glong cnt;
    // Check arguments
    g_return_val_if_fail (MDL_IS_UNIT (unit), FALSE);
    g_return_val_if_fail (text, FALSE);
    // Check filter string
    cnt = text ? g_utf8_strlen (text, -1) : 0;
    if (!cnt) return TRUE;
    // Get private
    priv = GET_PRIVATE (unit);
    // Check header
    reg = g_regex_new (g_strdup_printf ("%s", text), G_REGEX_CASELESS, 0, &err);
    returnValIfFailErr (reg, FALSE, "Regex create error : %s", err ? err->message : "Unknown");
    ok = g_regex_match (reg, priv->header, 0, NULL);
    if (ok) return TRUE;
    // Check text
    ok = g_regex_match (reg, priv->text, 0, NULL);
    if (ok) return TRUE;
    // Not found
    return FALSE;
}

gint unit_compare_units (gconstpointer a, gconstpointer b) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    // Check models
    g_return_val_if_fail (MDL_IS_UNIT (*((void**)a)), 0);
    g_return_val_if_fail (MDL_IS_UNIT (*((void**)b)), 0);

    #pragma GCC diagnostic pop
    // Get private structures
    MdlUnitPrivate *privA = GET_PRIVATE (*((void**)a));
    MdlUnitPrivate *privB = GET_PRIVATE (*((void**)b));
    // Prepare comparator strings
    gchar* strA = g_strdup_printf ("%s %s", privA->header, privA->text);
    gchar* strB = g_strdup_printf ("%s %s", privB->header, privB->text);
    // Convert strings to lowercase
    gchar* lowA = g_utf8_strdown(strA, -1);
    gchar* lowB = g_utf8_strdown(strB, -1);
    // Compare strings
    gint ret = g_strcmp0(lowA, lowB);
    // Free temporary strings
    g_free(lowA);
    g_free(lowB);
    g_free(strA);
    g_free(strB);

    return ret;
}

gboolean unit_equal_id (gconstpointer a, gconstpointer b) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    // Check models
    g_return_val_if_fail (MDL_IS_UNIT (a), FALSE);
    g_return_val_if_fail (MDL_IS_UNIT (b), FALSE);

    #pragma GCC diagnostic pop
    // Get private structures
    MdlUnitPrivate *privA = GET_PRIVATE (a);
    MdlUnitPrivate *privB = GET_PRIVATE (b);
    // Return TRUE if both units IDs are equal
    return (privA->roomId == privB->roomId && privA->userId == privB->userId) ? TRUE : FALSE;
}

guint unit_get_data(MdlUnit *mdl, GListStore *data) {
    // Variables
    guint i, cnt;
    gpointer it;
    MdlUnitPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_UNIT (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Get count
    cnt = ui_ptr_array_length (priv->members);
    // Copy all members
    selfLogTrc ("Copy %d member(s) to ListModel", cnt);
    for (i = 0; i < cnt; i++) {
        it = g_ptr_array_index (priv->members, i);
        selfLogTrc ("copy %s", unit_get_header (MDL_UNIT (it)));
        g_list_store_append (data, it);
    }
    return cnt;

}