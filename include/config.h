#ifndef CONFIG_H
#define CONFIG_H

#include "app.h"

int config_get_sounds (SoundData *open, SoundData *call);
int config_set_sounds (SoundData *open, SoundData *call);

#endif // CONFIG_H