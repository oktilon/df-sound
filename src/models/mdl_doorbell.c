/**
 * @file mdl_doorbell.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief
 * @version 0.1
 * @date 2023-08-17
 *
 *
 *
 */
#include "models/mdl_doorbell.h"
#include "download_manager.h"
#include "app.h"

typedef struct _MdlDoorbellPrivate {
    guint64 id;
    guint64 buildingId;
    mdlDoorbellConfig config;
    gchar *publicName;
    gchar *address;
    gchar *buildingsNumber;
    guint32 volume;
    guint32 brightness;
    guint32 timeToCloseModal;
    guint64 dialingSound;
    guint64 openDoorSound;
    GPtrArray *business_doors;
    GPtrArray *general_doors;
} MdlDoorbellPrivate;

/**
 * @brief Doorbell data parsed for services
 */
struct _MdlDoorbell {
    GObject parent_instance;
};

/*
    GListStore *business_doors;
    GListStore *general_doors;


*/

enum {
    PROP_0,
    PROP_ID,
    PROP_BUILDING_ID,
    PROP_PUBLIC_NAME,
    PROP_ADDRESS,
    PROP_BUILDINGS_NUMBER,
    PROP_BUSINESS_DOORS,
    PROP_GENERAL_DOORS,
    PROP_BUSINESS_COUNT,
    PROP_GENERAL_COUNT,

    N_PROPERTIES
};

static GParamSpec   *properties[N_PROPERTIES];

G_DEFINE_TYPE_WITH_PRIVATE (MdlDoorbell, doorbell, G_TYPE_OBJECT)

#define GET_PRIVATE(d) ((MdlDoorbellPrivate *) doorbell_get_instance_private ((MdlDoorbell*)d))

static void doorbell_init (MdlDoorbell *mdl) {
    MdlDoorbellPrivate *priv = GET_PRIVATE (mdl);

    priv->general_doors = g_ptr_array_new_full (0, g_object_unref);
    priv->business_doors = g_ptr_array_new_full (0, g_object_unref);
}

static void doorbell_dispose (GObject *object) {
    MdlDoorbellPrivate *priv = GET_PRIVATE (object);

    // free doors
    g_ptr_array_free (priv->general_doors, TRUE);
    priv->general_doors = NULL;

    g_ptr_array_free (priv->business_doors, TRUE);
    priv->business_doors = NULL;

    G_OBJECT_CLASS (doorbell_parent_class)->dispose (object);
}

static void doorbell_finalize (GObject *object) {
    MdlDoorbellPrivate *priv = GET_PRIVATE (object);

    if (priv->publicName) {
        g_free (priv->publicName);
    }
    if (priv->address) {
        g_free (priv->address);
    }
    if (priv->buildingsNumber) {
        g_free (priv->buildingsNumber);
    }

    G_OBJECT_CLASS (doorbell_parent_class)->finalize (object);
}

static void doorbell_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    MdlDoorbellPrivate *priv;

    g_return_if_fail (MDL_IS_DOORBELL (object));
    priv = GET_PRIVATE (object);

    switch (prop_id) {
        case PROP_ID:                   g_value_set_uint64 (value, priv->id); break;
        case PROP_BUILDING_ID:          g_value_set_uint64 (value, priv->buildingId); break;
        case PROP_PUBLIC_NAME:          g_value_take_string (value, priv->publicName); break;
        case PROP_ADDRESS:              g_value_take_string (value, priv->address); break;
        case PROP_BUILDINGS_NUMBER:     g_value_take_string (value, priv->buildingsNumber); break;
        case PROP_BUSINESS_DOORS:       g_value_set_pointer (value, priv->business_doors); break;
        case PROP_GENERAL_DOORS:       g_value_set_pointer (value, priv->general_doors); break;
        case PROP_BUSINESS_COUNT:       g_value_set_uint (value, ui_ptr_array_length (priv->business_doors)); break;
        case PROP_GENERAL_COUNT:        g_value_set_uint (value, ui_ptr_array_length (priv->general_doors)); break;
        default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); break;
    }
}

static void doorbell_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    g_return_if_fail (MDL_IS_DOORBELL (object));

    switch (prop_id) {

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void doorbell_class_init (MdlDoorbellClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = doorbell_dispose;
    object_class->finalize = doorbell_finalize;
    object_class->set_property = doorbell_set_property;
    object_class->get_property = doorbell_get_property;
    // Readonly
    properties[PROP_ID]                 = g_param_spec_uint64  ("id",               "id",               "id",   0, UINT64_MAX, 0,   G_PARAM_READABLE);
    properties[PROP_BUILDING_ID]        = g_param_spec_uint64  ("building-id",      "buildingId",       "bid",  0, UINT64_MAX, 0,   G_PARAM_READABLE);
    properties[PROP_PUBLIC_NAME]        = g_param_spec_string  ("name",             "publicName",       "name", "Doorbell",         G_PARAM_READABLE);
    properties[PROP_ADDRESS]            = g_param_spec_string  ("address",          "address",          "addr", "",                 G_PARAM_READABLE);
    properties[PROP_BUILDINGS_NUMBER]   = g_param_spec_string  ("buildings-number", "buildingsNumber",  "num",  "",                 G_PARAM_READABLE);
    properties[PROP_BUSINESS_DOORS]     = g_param_spec_pointer ("business-doors",   "businessDoors",    "bd",                       G_PARAM_READABLE);
    properties[PROP_GENERAL_DOORS]      = g_param_spec_pointer ("general-doors",    "generalDoors",     "gd",                       G_PARAM_READABLE);
    properties[PROP_BUSINESS_COUNT]     = g_param_spec_uint    ("business-count",   "businessCount",    "bc",   0, UINT32_MAX, 0,   G_PARAM_READABLE);
    properties[PROP_GENERAL_COUNT]      = g_param_spec_uint    ("general-count",    "generalCount",     "gc",   0, UINT32_MAX, 0,   G_PARAM_READABLE);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* Public API */

MdlDoorbell *doorbell_new (sd_bus_message *msg) {
    // Variables
    int r;
    MdlUnit *unit;
    MdlDoorbellPrivate *priv;
    const char *name, *addr, *num;
    MdlDoorbell *mdl = (MdlDoorbell *) g_object_new (MDL_TYPE_DOORBELL, NULL);

    if (!(MDL_IS_DOORBELL (mdl))) {
        selfLogErr ("Create model error: %m");
        return NULL;
    }

    if (msg == NULL) return mdl;

    priv = GET_PRIVATE (mdl);

    // Read doorbell self data
    r = sd_bus_message_read (msg, DOORBELL_SELF_SIGNATURE
        , &(priv->id)
        , &(priv->buildingId)
        , &(priv->config.config)
        , &(name)
        , &(addr)
        , &(num)
        , &(priv->volume)
        , &(priv->brightness)
        , &(priv->timeToCloseModal)
        , &(priv->dialingSound)
        , &(priv->openDoorSound)
    );
    dbusReturnOnFailErr (r, NULL, "Read message error (%d): %s", r, strerror (-r));

    priv->publicName = g_strdup (name);
    priv->address = g_strdup (addr);
    priv->buildingsNumber = g_strdup (num);

    // Enter business doors array container
    r = sd_bus_message_enter_container (msg, SD_BUS_TYPE_ARRAY, UNIT_STRUCT_SIGNATURE);
    dbusReturnOnFailErr (r, NULL, "Enter business doors array error (%d): %s", r, strerror (-r));

    r = 0;
    while (sd_bus_message_at_end (msg, FALSE) == 0) {
        unit = unit_parse_new (msg, FALSE);
        if (unit) {
            g_ptr_array_add (priv->business_doors , unit);
            r++;
        }
    }

    if (r) {
        selfLogTrc ("Sort business doors");
        g_ptr_array_sort (priv->business_doors, unit_compare_units);
    }

    // Exit business doors array container
    r = sd_bus_message_exit_container (msg);
    dbusReturnOnFailErr (r, NULL, "Close business doors array error (%d): %s", r, strerror (-r));


    // Enter general doors array container
    r = sd_bus_message_enter_container (msg, SD_BUS_TYPE_ARRAY, UNIT_STRUCT_SIGNATURE);
    dbusReturnOnFailErr (r, NULL, "Enter general doors array error (%d): %s", r, strerror (-r));

    r = 0;
    while (sd_bus_message_at_end (msg, FALSE) == 0) {
        unit = unit_parse_new (msg, FALSE);
        if (unit) {
            g_ptr_array_add (priv->general_doors , unit);
            r++;
        }
    }

    if (r) {
        selfLogTrc ("Sort general doors");
        g_ptr_array_sort (priv->general_doors, unit_compare_units);
    }

    // Exit general doors array container
    r = sd_bus_message_exit_container (msg);
    dbusReturnOnFailErr (r, NULL, "Close general doors array error (%d): %s", r, strerror (-r));


    selfLogInf ("Parser finished id=%d, bid=%d, pubName=%s", priv->id, priv->buildingId, priv->publicName);
    return mdl;
}

void doorbell_set_volume (MdlDoorbell *mdl, guint32 vol) {
    MdlDoorbellPrivate *priv;

    g_return_if_fail (MDL_IS_DOORBELL (mdl));
    priv = GET_PRIVATE (mdl);

    priv->volume = vol;
}

void doorbell_set_brightness (MdlDoorbell *mdl, guint32 br) {
    // Variable
    MdlDoorbellPrivate *priv;
    // Check model
    g_return_if_fail (MDL_IS_DOORBELL (mdl));
    priv = GET_PRIVATE (mdl);
    // Set brightness
    priv->brightness = br;
}

uint8_t doorbell_get_config (MdlDoorbell *mdl) {
    // Variable
    MdlDoorbellPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOORBELL (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Return doorbell config
    return priv->config.config;
}

const gchar * doorbell_get_name (MdlDoorbell *mdl) {
    // Variable
    MdlDoorbellPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOORBELL (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Return doorbell name
    return priv->publicName;
}

gboolean doorbell_is_menu_mode (MdlDoorbell *mdl) {
    // Variables
    MdlDoorbellPrivate *priv;
    uint cntBus, cntGen;
    // Check model
    g_return_val_if_fail (MDL_IS_DOORBELL (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Get door counts
    cntBus = ui_ptr_array_length (priv->business_doors);
    cntGen = ui_ptr_array_length (priv->general_doors);
    // If there are both arrays - we are in Menu mode
    return cntBus && cntGen ? TRUE : FALSE;
}

guint64 doorbell_get_call_sound (MdlDoorbell *mdl) {
    // Variable
    MdlDoorbellPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOORBELL (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Return Call (dialing) sound id
    return priv->dialingSound;
}

guint64 doorbell_get_door_sound (MdlDoorbell *mdl) {
    // Variable
    MdlDoorbellPrivate *priv;
    // Check model
    g_return_val_if_fail (MDL_IS_DOORBELL (mdl), 0);
    priv = GET_PRIVATE (mdl);
    // Return open door sound id
    return priv->openDoorSound;
}
