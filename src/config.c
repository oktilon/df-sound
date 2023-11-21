#include "config.h"

int config_get_sounds (SoundData *open, SoundData *call) {
    if (open) {
        open->id = 1;
        strcat (open->url, "%s", "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/1.wav");
    }

    if (call) {
        call->id = 10;
        strcat (call->url, "%s", "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/10.wav");
    }
    return 0;
}

int config_set_sounds (SoundData *open, SoundData *call) {
    return 0;
}
