#pragma once

#include <pthread.h>

#include "app.h"

int sound_start ();
int sound_play (SoundType soundId);
int sound_stop (SoundType soundId);
int sound_update (SoundData *soundData, int count);
AppState sound_state ();
void sound_playing (int *call, int *open);