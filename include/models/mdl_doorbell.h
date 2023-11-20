#ifndef MODELS_MDL_DOORBELL_H
#define MODELS_MDL_DOORBELL_H

#include <stdint.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <systemd/sd-bus.h>

#include "models/mdl_unit.h"

#define DOORBELL_SELF_SIGNATURE         "ttysssuuutt"
#define DOORBELL_DATA_SIGNATURE         DOORBELL_SELF_SIGNATURE "a" UNIT_STRUCT_SIGNATURE "a" UNIT_STRUCT_SIGNATURE

typedef union {
    uint8_t config;
    struct {
        uint8_t installationMode    :1;
        uint8_t enableImg           :1;
        uint8_t hiddenSearchBar     :1;
        uint8_t hiddenButtons       :1;
        uint8_t :4;
    };
} mdlDoorbellConfig;

G_BEGIN_DECLS

#define MDL_TYPE_DOORBELL   doorbell_get_type ()
G_DECLARE_FINAL_TYPE (MdlDoorbell, doorbell, MDL, DOORBELL, GObject)

MdlDoorbell      *doorbell_new (sd_bus_message *m);
void              doorbell_update         (MdlDoorbell *old, MdlDoorbell *new);
void              doorbell_set_volume     (MdlDoorbell *mdl, guint32 vol);
void              doorbell_set_brightness (MdlDoorbell *mdl, guint32 br);
uint8_t           doorbell_get_config     (MdlDoorbell *mdl);
const gchar      *doorbell_get_name       (MdlDoorbell *mdl);
gboolean          doorbell_is_menu_mode   (MdlDoorbell *mdl);
guint64           doorbell_get_call_sound (MdlDoorbell *mdl);
guint64           doorbell_get_door_sound (MdlDoorbell *mdl);

G_END_DECLS

#endif // MODELS_MDL_DOORBELL_H
