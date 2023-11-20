#ifndef MODELS_MDL_UNIT_H
#define MODELS_MDL_UNIT_H

#include <stdint.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <systemd/sd-bus.h>

#define UNIT_BASE_SIGNATURE             "tttttysssyyuu"
#define UNIT_MEMBER_STRUCT_SIGNATURE    "(" UNIT_BASE_SIGNATURE ")"
#define UNIT_DATA_SIGNATURE             UNIT_BASE_SIGNATURE "a" UNIT_MEMBER_STRUCT_SIGNATURE
#define UNIT_STRUCT_SIGNATURE           "(" UNIT_DATA_SIGNATURE ")"

/**
 * @brief Union to use bits for boolean
 */
typedef union {
    uint8_t config;
    struct {
        uint8_t disabled         :1;
        uint8_t disabledSchedule :1;
        uint8_t disableSequence  :1;
        uint8_t imageLoaded      :1;
    };
} mdlUnitConfig;

typedef union {
    uint8_t fullWeek;
    struct {
        uint8_t sunday      :1;
        uint8_t monday      :1;
        uint8_t tuesday     :1;
        uint8_t wednesday   :1;
        uint8_t thursday    :1;
        uint8_t friday      :1;
        uint8_t saturday    :1;
    };
} weekDays;

typedef enum UnitTypeEnum {
    UNIT_TYPE_INDIVIDUAL,
    UNIT_TYPE_UNIT,
    UNIT_TYPE_BUSINESS,
    UNIT_TYPE_FAMILY,
    UNIT_TYPE_FAMILY_SEQUENCE,
    UNIT_TYPE_FAMILY_GROUP
} UnitType;

G_BEGIN_DECLS

#define MDL_TYPE_UNIT   unit_get_type ()
G_DECLARE_FINAL_TYPE (MdlUnit, unit, MDL, UNIT, GObject)

MdlUnit        *unit_new                (guint64 room
                                       , guint64 user
                                       , const gchar *name);
MdlUnit        *unit_parse_new          (sd_bus_message *msg
                                       , int isMember);

void            unit_image_loaded       (MdlUnit *mdl);
gboolean        unit_is_image_loaded    (MdlUnit *mdl);
guint64         unit_get_room           (MdlUnit *mdl);
guint64         unit_get_user           (MdlUnit *mdl);
const gchar    *unit_get_header         (MdlUnit *mdl);
const gchar    *unit_get_text           (MdlUnit *mdl);
const gchar    *unit_get_image_url      (MdlUnit *mdl);
UnitType        unit_get_unit_type      (MdlUnit *mdl);
GPtrArray      *unit_get_members        (MdlUnit *mdl);
gboolean        unit_has_members        (MdlUnit *mdl);
gboolean        unit_contains_text      (gpointer *unit, const gchar *text);
gint            unit_compare_units      (gconstpointer a
                                       , gconstpointer b);
gboolean        unit_equal_id           (gconstpointer a
                                       , gconstpointer b);
guint           unit_get_data           (MdlUnit *mdl, GListStore *data);


G_END_DECLS

#endif // MODELS_MDL_UNIT_H