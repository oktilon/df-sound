#include "config.h"

int config_get_sounds (SoundShort **sounds) {
    SoundShort *data = (SoundShort *) calloc (sizeof(SoundShort), 2);

    // Open
    data[0].type = SoundOpen;
    data[0].id = 1;
    strcat (data[0].url, "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/1.wav");

    // Call
    data[1].type = SoundCall;
    data[1].id = 10;
    strcat (data[1].url, "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/10.wav");

    *sounds = data;

    return 2;
}

int config_set_sounds (SoundShort *sounds, int count) {
    return TRUE;
}
