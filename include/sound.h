#pragma once

#include <pthread.h>

#include "app.h"

int sound_start ();
int sound_play (uint64_t soundId, int single);
int sound_stop (uint64_t soundId);
int sound_update (uint64_t idOpen, const char *urlOpen, uint64_t idCall, const char *urlCall);