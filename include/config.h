#ifndef CONFIG_H
#define CONFIG_H

#include "app.h"

int config_get_sounds (SoundShort **sounds);
int config_set_sounds (SoundShort *sounds, int count);
void config_free ();

#endif // CONFIG_H