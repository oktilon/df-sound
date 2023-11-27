#include <math.h>

#include "app.h"
#include "mixer.h"

static int sset_enum (snd_mixer_elem_t *elem, uint8_t vol);
static int sset_channels (snd_mixer_elem_t *elem, uint8_t vol);
static int set_volume_simple (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t chn, uint8_t vol);


static char card[64] = "default";

int mixer_set_volume (uint8_t vol) {
    int err = 0;
    snd_mixer_t *handle = NULL;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *sid;

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

    /* enum control */
    if (snd_mixer_selem_is_enumerated(elem))
        err = sset_enum (elem, vol);
    else
        err = sset_channels (elem, vol);

    if (err < 0) {
        selfLogErr ("Invalid command!"); // ?????
    } else if (err) {
        selfLogInf ("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));
        // show_selem(handle, sid, "  ", 1);
    }

    snd_mixer_close (handle);
    return 0;
}

uint8_t mixer_get_volume () {
    return 32;
}

static int sset_enum(snd_mixer_elem_t *elem, uint8_t vol) {
    selfLogInf ("sset_enum");
    return 0;
}

static int sset_channels(snd_mixer_elem_t *elem, uint8_t vol) {
    selfLogInf ("sset_channels");
	unsigned int channels = ~0U; // All channels
	unsigned int dir = 3; // 1-playback, 2-capture
	unsigned int okflag = 3;
	unsigned int idx;
	snd_mixer_selem_channel_id_t chn;
	int check_flag = -1;
    int multi = 0;

    for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
        char *sptr = NULL;
        int ival;

        // Playback
        if (snd_mixer_selem_has_playback_channel (elem, chn)) {
            if (set_volume_simple(elem, chn, vol) >= 0)
                check_flag = 1;
        }

    }

    return 0;
}

static int set_volume_simple(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t chn, uint8_t vol) {
    int err;
    long val, pMin, pMax, tmp;

    err = snd_mixer_selem_has_playback_volume (elem);
    selfLogDbg ("snd_mixer_selem_has_playback_volume return %d", err);

    // get_range = snd_mixer_selem_get_playback_volume_range
    // get = snd_mixer_selem_get_playback_volume
    // set = snd_mixer_selem_set_playback_volume

    err = snd_mixer_selem_get_playback_volume_range (elem, &pMin, &pMax);

    // Convert percent to value within range
    tmp = rint ((double)vol * (double)(pMax - pMin) * 0.01);
    if (tmp == 0 && vol > 0)
        tmp++;
    val = tmp + pMin;

    //
    err = snd_mixer_selem_set_playback_volume (elem, chn, val);
}