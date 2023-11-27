/**
 * @file sound.c
 * @author Denys Stovbun (denis.stovbun@lanars.com)
 * @brief Processing sound files
 * @version 0.1
 * @date 2023-11-24
 *
 *
 *
 */
#define _GNU_SOURCE         /* asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

#include "bus.h"
#include "sound.h"
#include "config.h"
#include "formats.h"
#include "download.h"

#define SOUNDS_FOLDER           ""

typedef struct PlayDataStruct {
    SoundData *soundData;
    snd_pcm_t *pcm;
} PlayData;

static void sound_check_and_update (SoundData *data, SoundShort *newData);
static void play_audio(PlayData *play);
static void *play_wave(void* ptr);
static void parse_wave_file (int file, WaveHeader *h, SoundData *data);
static int load_wave_file (SoundData *data);
static void set_state (AppState newState);
static void set_playing (SoundType type);
static void reset_playing (SoundType type);
static const char * sound_type (SoundType type);
static const char * app_state (AppState state);

static const char       SoundCardPortName[] = "default";
static SoundData        soundOpen   = { SoundOpen };
static SoundData        soundCall   = { SoundCall };
static pthread_t        threadOpen  = 0UL;
static pthread_t        threadCall  = 0UL;
static int              playingOpen = FALSE;
static int              playingCall = FALSE;
static AppState         state       = SND_Initializing;

int sound_start () {
    int i, r, cnt;
    SoundShort *data = NULL;

    // Dbus init
    r = dbus_init ();
    if (r < 0) return r;

    // Read config
    cnt = config_get_sounds (&data);
    if (cnt) {
        for (i = 0; i < cnt; i++) {
            selfLogTrc ("%s[%d] id=%d", sound_type (data[i].type), data[i].type, data[i].id);
            switch (data[i].type) {
                case SoundOpen:
                    sound_check_and_update (&soundOpen, data + i);
                    break;

                case SoundCall:
                    sound_check_and_update (&soundCall, data + i);
                    break;

                default:
                    selfLogWrn ("Wrong sound type: %d", data[i].type);
                    break;
            }
        }
    }

    if (state == SND_Initializing)
        set_state (SND_Idle);

    while (!r) {
        r = download_count ();
        if (state == SND_Downloading && r == 0) {
            // parse files
            set_state (SND_Idle);
        }
        r = dbus_loop ();
    }

    return r;
}

int sound_play (SoundType soundType) {
    int r;
    pthread_t *th = NULL;
    SoundData *data = NULL;

    switch (soundType) {
        case SoundOpen:
            th = &threadOpen;
            data = &soundOpen;
            break;

        case SoundCall:
            th = &threadCall;
            data = &soundCall;
            break;

        default:
            selfLogWrn ("Wrong sound type: %d", soundType);
            return FALSE;
    }

    if (!data) {
        selfLogWrn ("Sound type %d [%s] is not initialized!", soundType, sound_type (soundType));
        return FALSE;
    }

    if (*th) {
        pthread_cancel (*th);
        *th = 0;
    }
    r = pthread_create (th, NULL, play_wave, data);
    if (r) {
        selfLogErr ("Create thread error(%d): %m", errno);
        return FALSE;
    }

    return TRUE;
}

int sound_stop (SoundType soundType) {
    pthread_t *th = NULL;
    int *playing = NULL;

    switch (soundType) {
        case SoundOpen:
            th = &threadOpen;
            playing = &playingOpen;
            break;

        case SoundCall:
            th = &threadCall;
            playing = &playingCall;
            break;

        default:
            selfLogWrn ("Wrong sound type: %d", soundType);
            return FALSE;
    }

    if (th && *th) {
        pthread_cancel (*th);
        *th = 0;

        *playing = FALSE;
        dbus_emit_playing ();
    }
    if (state == SND_Playing && !playingCall && !playingOpen)
        set_state (SND_Idle);

    return TRUE;
}

int sound_update (SoundShort *soundData, int count) {
    int i;
    selfLogInf ("Update %d sound(s)", count);

    for (i = 0; i < count; i++) {
        selfLogInf ("Update #%d %s sound id=%d", i + 1, sound_type (soundData[i].type), soundData[i].id);
        switch (soundData[i].type) {
            case SoundOpen:
                sound_check_and_update (&soundOpen, soundData + i);
                break;

            case SoundCall:
                sound_check_and_update (&soundCall, soundData + i);
                break;

            default:
                selfLogWrn ("Unknown sound type: %d", soundData[i].type);
                break;
        }
    }

    return config_set_sounds (soundData, count);
}

AppState sound_state () {
    return state;
}

const char * sound_state_name () {
    return app_state (state);
}

void sound_playing (int *call, int *open) {
    *call = playingCall;
    *open = playingOpen;
}

static void sound_check_and_update (SoundData *data, SoundShort *newData) {
    int r;
    selfLogInf ("Update %s sound [old=%d, new=%d] url's %s equal", sound_type (data->type), data->id, newData->id, strcmp (data->url, newData->url) == 0 ? "are" : "aren't");
    // If update is not required
    if (data->id == newData->id && strcmp (data->url, newData->url) == 0 && data->data)
        return;

    // Clean old data
    if (data->data) {
        free (data->data);
        data->data = NULL;
    }

    // Copy SoundData
    memset (data, 0, sizeof (SoundData));
    data->type = newData->type;
    data->id = newData->id;
    strcpy (data->url, newData->url);

    if (!strlen (data->url)) // No sound
        return;

    // Create sound file name
    snprintf (data->filename, MAX_FILE_SIZE, "%s/%s%ld.wav", getenv ("HOME"), SOUNDS_FOLDER, data->id);

    // Test if file not exists
    if (access (data->filename, F_OK)) {
        selfLogTrc ("Start download [%s] %s", data->filename, data->url);
        r = download_sound (data);
        if (r)
            set_state (SND_Downloading);

        return;
    }

    // Parse file
    load_wave_file (data);
}

static void parse_wave_file (int file, WaveHeader *h, SoundData *data) {
    uint8_t         buf[64] = {0};
    WaveChunkHeader c;
    WaveFmtBody    *f = (WaveFmtBody *)buf;
    int             r;
    int             bigEndian;
    uint8_t         vbps = 0;
    uint16_t        format;
    uint32_t        len;

    if (h->magic == WAV_RIFF)
        bigEndian = 0;
    else if (h->magic == WAV_RIFX)
        bigEndian = 1;
    else {
        selfLogErr ("Is not a RIFF/X file [%s]", data->filename);
        return;
    }

    returnIfFailErr (h->type == WAV_WAVE, "Is not a WAVE file [%s] type is [%c%c%c%c]", data->filename, DUMP_ID (h->type));

    selfLogDbg ("File [%s] hdr: ID=%c%c%c%c, Type=%c%c%c%c, Len=%d", data->filename, DUMP_ID (h->magic), DUMP_ID (h->type), h->length);

    // Read in next chunk header
    while (read (file, &c, sizeof (WaveChunkHeader)) == sizeof (WaveChunkHeader)) {
        char *log = NULL;

        len = TO_CPU_INT (c.length, bigEndian);

        // ============================ Is it a fmt chunk? ===============================
        if (WAV_FMT == c.type) {
            char chan[12] = {0};


            if (read (file, &buf, len) != len) {
                selfLogErr ("Read format error(%d): %m", errno);
                break;
            }

            format = TO_CPU_SHORT (f->format, bigEndian);

            if (WAV_FMT_EXTENSIBLE == format) {
                WaveFmtExtensibleBody *fe = (WaveFmtExtensibleBody*) buf;

                returnIfFailErr (len >= sizeof(WaveFmtExtensibleBody)
                    , "unknown length of extensible 'fmt ' chunk (read %u, should be %u at least)"
                    , len
                    , (unsigned int)sizeof(WaveFmtExtensibleBody));

                if (memcmp(fe->guid_tag, WAV_GUID_TAG, 14) != 0) {
                    selfLogErr ("Wrong format tag in extensible 'fmt ' chunk");
                    return;
                }
                vbps = TO_CPU_SHORT(fe->bit_p_spl, bigEndian);
                format = TO_CPU_SHORT(fe->guid_format, bigEndian);
            }

            // Can't handle compressed WAVE files
            returnIfFailErr ((format == WAV_FMT_PCM || format == WAV_FMT_IEEE_FLOAT)
                , "Unsupported WAVE-file format 0x%04x which is not PCM or FLOAT encoded"
                , format);

            data->channels = TO_CPU_SHORT (f->channels, bigEndian);
            returnIfFailErr (data->channels >= 1, "Can't play WAVE-files with %d tracks", data->channels);
            if (data->channels == 1)
                sprintf (chan, "Mono");
            else if (data->channels == 2)
                sprintf (chan, "Stereo");
            else
                sprintf (chan, "%dch.", data->channels);

            data->rate = TO_CPU_INT(f->sample_fq, bigEndian);
            data->align = TO_CPU_SHORT (f->byte_p_spl, bigEndian);

            data->bits = TO_CPU_SHORT (f->bit_p_spl, bigEndian);
            returnIfFailErr (vbps <= data->bits, "Valid bps greater than bps: %d > %d", vbps, data->bits);

            switch (data->bits) {
            case 8:
                data->format = SND_PCM_FORMAT_U8;
                r = asprintf (&log, "PCM Unsigned 8bit freq:%u %s", data->rate, chan);
                break;
            case 16:
                if (bigEndian) {
                    data->format = SND_PCM_FORMAT_S16_BE;
                    r = asprintf (&log, "PCM Signed 16bit Big endian freq:%u %s", data->rate, chan);
                } else {
                    data->format = SND_PCM_FORMAT_S16_LE;
                    r = asprintf (&log, "PCM Signed 16bit Little endian freq:%u %s", data->rate, chan);
                }
                break;
            case 24:
                switch (data->align / data->channels) {
                case 3:
                    if (bigEndian) {
                        data->format = SND_PCM_FORMAT_S24_3BE;
                        r = asprintf (&log, "PCM Signed 24bit Big endian in 3bytes freq:%u %s", data->rate, chan);
                    } else {
                        data->format = SND_PCM_FORMAT_S24_3LE;
                        r = asprintf (&log, "PCM Signed 24bit Little endian in 3bytes freq:%u %s", data->rate, chan);
                    }
                    break;
                case 4:
                    if (bigEndian) {
                        data->format = SND_PCM_FORMAT_S24_BE;
                        r = asprintf (&log, "PCM Signed 24bit Big endian freq:%u %s", data->rate, chan);
                    } else {
                        data->format = SND_PCM_FORMAT_S24_LE;
                        r = asprintf (&log, "PCM Signed 24bit Little endian freq:%u %s", data->rate, chan);
                    }
                    break;
                default:
                    selfLogErr("Can't play WAVE-files with sample %d bits in %d bytes wide (%d channels)",
                        data->bits,
                        data->align,
                        data->channels);
                    return;
                }
                break;
            case 32:
                if (format == WAV_FMT_PCM) {
                    switch (vbps) {
                    case 24:
                        if (bigEndian) {
                            data->format = SND_PCM_FORMAT_S24_BE;
                            r = asprintf (&log, "PCM Signed 24bit Big endian freq:%u %s", data->rate, chan);
                        } else {
                            data->format = SND_PCM_FORMAT_S24_LE;
                            r = asprintf (&log, "PCM Signed 24bit Little endian freq:%u %s", data->rate, chan);
                        }
                        break;
                    default:
                        if (bigEndian) {
                            data->format = SND_PCM_FORMAT_S32_BE;
                            r = asprintf (&log, "PCM Signed 32bit Big endian freq:%u %s", data->rate, chan);
                        } else {
                            data->format = SND_PCM_FORMAT_S32_LE;
                            r = asprintf (&log, "PCM Signed 32bit Little endian freq:%u %s", data->rate, chan);
                        }
                        break;
                    }
                } else if (format == WAV_FMT_IEEE_FLOAT) {
                    if (bigEndian) {
                        data->format = SND_PCM_FORMAT_FLOAT_BE;
                        r = asprintf (&log, "PCM Float Big endian freq:%u %s", data->rate, chan);
                    } else {
                        data->format = SND_PCM_FORMAT_FLOAT_LE;
                        r = asprintf (&log, "PCM Float Little endian freq:%u %s", data->rate, chan);
                    }
                }
                break;
            default:
                selfLogErr ("Can't play WAVE-files with sample %d bits wide",
                    data->bits);
                return;
            }
        }

        // ============================ Is it a data chunk? ===============================
        else if (WAV_DATA == c.type) {
            ssize_t sz;

            len += len % 2;
            // Size of wave data is c.length. Allocate a buffer and read in the wave data
            if (!(data->data = (uint8_t *) malloc (len))) {
                selfLogErr ("Allocate memory error: %m");
                break;
            }

            sz = read (file, data->data, len);
            if (sz != len) {
                free (data->data);
                data->data = NULL;
                selfLogErr ("Data chunk sz=%ld not equal c.length=%u", sz, len);
                break;
            }

            // Store size (in frames)
            data->size = (len * 8) / ((uint32_t) data->bits * (uint32_t) data->channels);

            r = asprintf (&log, "data has %ld frames", data->size);
        }

        // ============================ Skip this chunk ===============================
        else {
            len += len % 2;
            r = asprintf (&log, "skip it [%d bytes]", len);
            lseek(file, len, SEEK_CUR);
        }

        selfLogTrc ("Chunk hdr: ID=%c%c%c%c, Len=%d : %s", DUMP_ID (c.type), len, log);
        if (log && r) free (log);
    }
}

static int load_wave_file (SoundData *data) {
    WaveHeader      h;
    int             file;

    if (!data) {
        selfLogErr ("Invalid pointer");
        return FALSE;
    }

    data->data = NULL;
    data->channels = 0;

    if ((file = open (data->filename, O_RDONLY)) == -1) {

        selfLogErr ("Can't open file [%s]: %m", data->filename);
        return FALSE;

    } else {

        if (read (file, &h, sizeof (WaveHeader)) == sizeof (WaveHeader))
            parse_wave_file (file, &h, data);
        else
            selfLogErr ("Read file [%s] header error(%d): %m", data->filename, errno);

        close (file);
    }

    return data->data && data->format && data->channels ? TRUE : FALSE;
}


static void play_audio (PlayData *play) {
    register snd_pcm_uframes_t count, frames;

    // Output the wave data
    count = 0;
    do {
        frames = snd_pcm_writei (play->pcm, play->soundData->data + count, play->soundData->size - count);
        selfLogTrc ("writen %ld frames of %ld left", frames, play->soundData->size - count);

        // If an error, try to recover from it
        if (frames < 0)
            frames = snd_pcm_recover(play->pcm, frames, 0);
        if (frames < 0) {
            selfLogErr ("Error playing wave: %s", snd_strerror(frames));
            break;
        }

        // Update our pointer
        count += frames;

    } while (count < play->soundData->size);

    // Wait for playback to completely finish
    //if (count == WaveSize)
        //snd_pcm_drain(PlaybackHandle);
}

static void * play_wave(void* ptr) {
    // No wave data loaded yet
    register int err;
    PlayData play = { (SoundData *) ptr, NULL };

    returnValIfFailWrn ((play.soundData->format && play.soundData->channels), NULL, "No sound data");

    // Open audio card we wish to use for playback
    err = snd_pcm_open (&(play.pcm), &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
        selfLogErr ("Can't open audio %s: %s", &SoundCardPortName[0], snd_strerror (err));
    else {

        // Set the audio card's hardware parameters (sample rate, bit resolution, etc)
        err = snd_pcm_set_params (play.pcm
            , play.soundData->format        // Format
            , SND_PCM_ACCESS_RW_INTERLEAVED // Access
            , play.soundData->channels      // Channels
            , play.soundData->rate          // Rate
            , 1                             // Soft resample
            , 100000);                      // Latency

        if (err < 0)
            selfLogErr ("Can't set sound parameters: %s", snd_strerror(err));
        else {
            set_playing (play.soundData->type);
            play_audio (&play);
        }

        snd_pcm_drain (play.pcm);
        snd_pcm_close (play.pcm);

        reset_playing (play.soundData->type);
    }

    pthread_exit (NULL);
}

static void set_state (AppState newState) {
    if (state == newState) return;
    state = newState;
    dbus_emit_state ();
}

static void set_playing (SoundType type) {
    int emit = FALSE;
    if (state != SND_Playing) set_state (SND_Playing);
    if (!playingCall && type == SoundCall) {
        playingCall = TRUE;
        emit = TRUE;
    }
    if (!playingOpen && type == SoundOpen) {
        playingOpen = TRUE;
        emit = TRUE;
    }
    if (emit) dbus_emit_playing (playingCall, playingOpen);
}

static void reset_playing (SoundType type) {
    int emit = FALSE;
    if (playingCall && type == SoundCall) {
        playingCall = FALSE;
        emit = TRUE;
    }
    if (playingOpen && type == SoundOpen) {
        playingOpen = FALSE;
        emit = TRUE;
    }
    if (emit) dbus_emit_playing (playingCall, playingOpen);
    if (!playingCall && !playingOpen && state != SND_Idle) set_state (SND_Idle);
}

static const char * sound_type (SoundType type) {
    switch (type) {
        case SoundNone: return "None";
        case SoundOpen: return "Open";
        case SoundCall: return "Call";
        default: break;
    }
    return "Unknown";
}

static const char * app_state (AppState state) {
    switch (state) {
        case SND_Initializing:  return "Initializing";
        case SND_Downloading:   return "Downloading";
        case SND_Idle:          return "Idle";
        case SND_Playing:       return "Playing";
        default: break;
    }
    return "Unknown";
}