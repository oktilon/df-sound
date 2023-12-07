#pragma once

#include <pthread.h>

#include "app.h"

int sound_start_service ();
int sound_play (SoundType soundId);
int sound_stop (SoundType soundId);
int sound_update (SoundShort *soundData, int count);
void sound_playing (int *call, int *open);
AppState sound_state ();
const char * sound_state_name ();