/**
 * @file mixer.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief Mixer controls (volume)
 * @version 0.1
 * @date 2023-11-27
 *
 *
 *
 */
#include <math.h>

#include "app.h"
#include "mixer.h"

static int mixer_process_volume (uint8_t *vol, int readValue);
static void mixer_set_playback_volume (snd_mixer_elem_t *elem, long val);
static long mixer_get_playback_volume (snd_mixer_elem_t *elem);
static long convert_to_raw (snd_mixer_elem_t *elem, uint8_t vol);
static uint8_t convert_to_percent (snd_mixer_elem_t *elem, long val);


int mixer_set_volume (uint8_t vol) {
    return mixer_process_volume (&vol, FALSE);
}

uint8_t mixer_get_volume () {
    uint8_t vol = 0;

    mixer_process_volume (&vol, TRUE);

    return vol;
}

static int mixer_process_volume (uint8_t *vol, int readValue) {
    int err = 0;
    snd_mixer_t *handle = NULL;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;
    long val = 0;


    snd_mixer_selem_id_alloca (&sid);
    snd_mixer_selem_id_set_index (sid, 0);
    snd_mixer_selem_id_set_name (sid, "Headphone");

    err =  snd_mixer_open (&handle, 0);
    returnValIfFailErr (DBUS_OK (err), err, "Mixer %s open error: %s", card, snd_strerror (err));

    err = snd_mixer_attach (handle, card);
    returnValIfFailErr (DBUS_OK (err), err, "Mixer attach %s error: %s", card, snd_strerror (err));

    err = snd_mixer_selem_register (handle, NULL, NULL);
    if (err < 0) {
        selfLogErr ("Mixer register error: %s", snd_strerror (err));
        snd_mixer_close (handle);
        return err;
    }

    err = snd_mixer_load (handle);
    if (err < 0) {
        selfLogErr ("Mixer %s load error: %s", card, snd_strerror (err));
        snd_mixer_close (handle);
        return err;
    }

    elem = snd_mixer_find_selem (handle, sid);
    if (!elem) {
        selfLogErr ("Unable to find simple control '%s',%i", snd_mixer_selem_id_get_name (sid), snd_mixer_selem_id_get_index (sid));
        snd_mixer_close(handle);
        return -ENOENT;
    }

    err = snd_mixer_selem_has_playback_volume (elem);
    if (err <= 0) {
        selfLogWrn ("Simple control doesn't have Playback volume");
        snd_mixer_close(handle);
        return -EINVAL;
    }

    if (readValue) {
        val = mixer_get_playback_volume (elem);
        *vol = convert_to_percent (elem, val);
    } else {
        val = convert_to_raw (elem, *vol);
        if (val < 0)
            return -EPERM;
        mixer_set_playback_volume (elem, val);
    }

    snd_mixer_close (handle);
    return 0;
}

static void mixer_set_playback_volume (snd_mixer_elem_t *elem, long val) {
    int err;
    snd_mixer_selem_channel_id_t chn;

    for (chn = 0; chn <= SND_MIXER_SCHN_REAR_CENTER; chn++) {
        if (snd_mixer_selem_has_playback_channel (elem, chn)) {
            err = snd_mixer_selem_set_playback_volume (elem, chn, val);
            if (err < 0) {
                selfLogWrn ("Set channel:%d volume error(%d): %s", chn, err, snd_strerror (err));
                continue;
            } else
                selfLogTrc ("Channel:%d volume=%d", chn, val);
        }
    }
}

static long mixer_get_playback_volume (snd_mixer_elem_t *elem) {
    int err;
    long val = 0;
    snd_mixer_selem_channel_id_t chn;

    for (chn = 0; chn <= SND_MIXER_SCHN_REAR_CENTER; chn++) {
        if (snd_mixer_selem_has_playback_channel (elem, chn)) {
            err = snd_mixer_selem_get_playback_volume (elem, chn, &val);
            if (err < 0) {
                selfLogWrn ("Get channel:%d volume error(%d): %s", chn, err, snd_strerror (err));
                continue;
            } else {
                selfLogTrc ("Channel:%d volume=%d", chn, val);
                break;
            }
        }
    }

    return val;
}

static long convert_to_raw (snd_mixer_elem_t *elem, uint8_t vol) {
    int err;
    long val, pMin, pMax, tmp;

    err = snd_mixer_selem_get_playback_volume_range (elem, &pMin, &pMax);
    if (err < 0) {
        selfLogErr ("Get volume range error(%d): %s", err, snd_strerror (err));
        return -EPERM;
    }

    // Convert percent to value within range
    tmp = rint ((double)vol * (double)(pMax - pMin) * 0.01);
    if (tmp == 0 && vol > 0)
        tmp++;
    val = tmp + pMin;
    // Check over range
    val = val < pMin ? pMin : (val > pMax ? pMax : val);
    selfLogDbg ("Percent [%d] to val=%d", vol, val);

    return val;
}

static uint8_t convert_to_percent (snd_mixer_elem_t *elem, long val) {
    int err;
    uint8_t vol = 0;
    long pMin, pMax, dVal;

    err = snd_mixer_selem_get_playback_volume_range (elem, &pMin, &pMax);
    if (err < 0) {
        selfLogErr ("Get volume range error(%d): %s", err, snd_strerror (err));
        return -EPERM;
    }

    // Convert percent to value within range
    dVal = pMax - pMin;
    if (dVal) {
        vol = rint (100.0 * ((double)val - (double)pMin) / (double)dVal);
    }
    selfLogDbg ("Val [%d] to percent %d%%", val, vol);

    return vol;
}