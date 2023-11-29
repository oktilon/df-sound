#ifndef CONFIG_H
#define CONFIG_H

#include "app.h"

int config_read_data (SoundShort **sounds);
int config_write_data (SoundShort *sounds, int count);

#endif // CONFIG_H