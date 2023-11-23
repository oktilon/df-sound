// A simple C example to play a mono or stereo, 16-bit 44KHz
// WAVE file using ALSA. This goes directly to the first
// audio card (ie, its first set of audio out jacks). It
// uses the snd_pcm_writei() mode of outputting waveform data,
// blocking.
//
// Compile as so to create "alsawave":
// gcc -o alsawave alsawave.c -lasound
//
// Run it from a terminal, specifying the name of a WAVE file to play:
// ./alsawave MyWaveFile.wav
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

// Include the ALSA .H file that defines ALSA functions/data

#include "bus.h"
#include "sound.h"
#include "config.h"
#include "formats.h"
#include "download.h"

typedef struct PlayDataStruct {
    SoundData *soundData;
    snd_pcm_t *pcm;
} PlayData;

static void sound_check_and_update (SoundData *data, SoundShort *newData);
static void play_audio(PlayData *play);
static void *play_wave(void* ptr);
static int load_wave_file (SoundData *data);
static void set_state (AppState newState);
static void set_playing (SoundType type);
static void reset_playing (SoundType type);

static const char       SoundCardPortName[] = "default";
static SoundData        soundOpen   = {0};
static SoundData        soundCall   = {0};
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
            selfLogTrc ("readed %d with id:%d", data[i].type, data[i].id);
            switch (data[i].type) {
                case SoundOpen:
                    sound_check_and_update (&soundOpen, data + i);
                    break;

                case SoundCall:
                    sound_check_and_update (&soundCall, data + i);
                    break;

                default:
                    selfLogWrn ("Unknown sound type: %d", data[i].type);
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
    int *playing = NULL;

    switch (soundType) {
        case SoundOpen:
            th = &threadOpen;
            data = &soundOpen;
            playing = &playingOpen;
            break;

        case SoundCall:
            th = &threadCall;
            data = &soundCall;
            playing = &playingCall;
            break;

        default:
            selfLogWrn ("Invalid sound type: %d", soundType);
            return FALSE;
    }

    if (data) {
        if (*th) {
            // kill thread
        }
        r = pthread_create (th, NULL, play_wave, data);
        if (r) {
            selfLogErr ("Create thread error(%d): %m", errno);
            return FALSE;
        }
        *playing = TRUE;
        dbus_emit_playing ();
    } else {
        selfLogWrn ("Sound type %d isn't initialized!");
        return FALSE;
    }

    // Await 2nd threads return
    // pthread_join(*wait, NULL);

    // Close Sound Card
    // snd_pcm_drain(PlaybackHandle);
    // snd_pcm_close(PlaybackHandle);
    // free_wave_data();

    return TRUE;
}

int sound_stop (SoundType soundType) {
    pthread_t *th = NULL;

    switch (soundType) {
        case SoundOpen:
            th = &threadOpen;
            break;

        case SoundCall:
            th = &threadCall;
            break;

        default:
            selfLogWrn ("Invalid sound type: %d", soundType);
            return FALSE;
    }

    if (th && *th) {
        // pthread_
    }
    return TRUE;
}

int sound_update (SoundShort *soundData, int count) {
    int i;

    for (i = 0; i < count; i++) {
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

void sound_playing (int *call, int *open) {
    *call = playingCall;
    *open = playingOpen;
}

static void sound_check_and_update (SoundData *data, SoundShort *newData) {
    int r;
    selfLogTrc ("Got t:%d, id:%d old was id:%d [url:%s]", newData->type, newData->id, data->id, newData->url);
    // If update is not required
    if (data->id == newData->id && strcmp (data->url, newData->url) == 0)
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
    snprintf (data->filename, MAX_FILE_SIZE, "%ld.wav", data->id);

    selfLogTrc ("Filename is:[%s]", data->filename);

    // Test if file not exists
    if (access (data->filename, F_OK)) {
        selfLogTrc ("Start download [%s] %s", data->filename, data->url);
        r = download_sound (&soundOpen);
        if (r)
            set_state (SND_Downloading);

        return;
    }

    // Parse file
    load_wave_file (data);
}

static int load_wave_file (SoundData *data) {
    WaveChunkHeader  head;
    WaveHeader       headFile;
    register int     inHandle;

    if (!data) {
        selfLogErr ("Invalid pointer");
        return FALSE;
    }

    data->data = NULL;
    data->channels = 0;

    selfLogTrc ("Load wave [%d]: %s", data->id, data->filename);

    if ((inHandle = open (data->filename, O_RDONLY)) == -1) {

        selfLogErr ("Can't open file [%s]: %m", data->filename);
        return FALSE;

    } else {

        if (read (inHandle, &headFile, sizeof (WaveHeader)) == sizeof (WaveHeader)) {

            if (WAV_RIFF == headFile.magic && WAV_WAVE == headFile.type) {
                selfLogTrc ("File hdr: ID=0x%08X, Type=0x%08X, Len=%d\n", headFile.magic, headFile.type, headFile.length);

                // Read in next chunk header
                while (read (inHandle, &head, sizeof (WaveChunkHeader)) == sizeof (WaveChunkHeader)) {

                    selfLogTrc ("Chunk hdr: ID=0x%08X, Len=%d : ", head.type, head.length);
                    // ============================ Is it a fmt chunk? ===============================
                    if (WAV_FMT == head.type) {
                        WaveFmtExtensibleBody  formatEx;

                        // Read in the remainder of chunk
                        if (read (inHandle, &formatEx, head.length) != head.length) {
                            selfLogTrc ("Read error: %m");
                            break;
                        }

                        // Can't handle compressed WAVE files
                        if (formatEx.format.format != WAV_FMT_PCM) {
                            selfLogTrc ("Unsupported WAVE format: 0x%04X", formatEx.format.format);
                            break;
                        }

                        data->bits  = (uint8_t)formatEx.format.bit_p_spl;
                        data->rate  = (uint16_t)formatEx.format.sample_fq;
                        data->align = (uint8_t)formatEx.format.byte_p_spl;
                        data->channels = formatEx.format.channels;

                        switch (data->bits) {
                            case 8:
                                data->format = SND_PCM_FORMAT_U8;
                                break;

                            case 16:
                                data->format = SND_PCM_FORMAT_S16_LE;
                                break;

                            case 24:
                                if ((data->align % 3) == 0) {
                                    data->format = SND_PCM_FORMAT_S24_3LE;
                                } else {
                                    data->format = SND_PCM_FORMAT_S24;
                                }
                                break;

                            case 32:
                                data->format = SND_PCM_FORMAT_S32_LE;
                                break;

                            default:
                                data->format = SND_PCM_FORMAT_U8;
                                break;
                        }
                    }

                    // ============================ Is it a data chunk? ===============================
                    else if (WAV_DATA == head.type) {
                        ssize_t sz;
                        // Size of wave data is head.length. Allocate a buffer and read in the wave data
                        if (!(data->data = (uint8_t *) malloc (head.length))) {
                            selfLogTrc ("Allocate memory error: %m");
                            break;
                        }

                        sz = read (inHandle, data->data, head.length);
                        if (sz != head.length) {
                            free (data->data);
                            data->data = NULL;
                            selfLogErr ("Data chunk sz=%ld not equal head.length=%u\n", sz, head.length);
                            break;
                        }

                        // Store size (in frames)
                        data->size = (head.length * 8) / ((uint32_t)data->bits * (uint32_t)data->channels);

                        selfLogTrc ("Data chunk sz=%ld, frames=%ld\n", sz, data->size);
                    }

                    // ============================ Skip this chunk ===============================
                    else {
                        selfLogTrc ("Skip chunk sz=%d\n", head.length);
                        if (head.length & 1) head.length++;
                        lseek(inHandle, head.length, SEEK_CUR);
                    }
                }
            } else {
                selfLogErr ("Is not a WAVE file [%s]: %m", data->filename);
            }

        } else {
            selfLogErr ("Can't open file [%s]: %m", data->filename);
        }

        if (!data->data || !data->channels) {
            selfLogErr ("It is a bad WAVE file [%s]", data->filename);
        }

        close (inHandle);
    }

    return data->data && data->channels ? TRUE : FALSE;
}


static void play_audio (PlayData *play) {
    register snd_pcm_uframes_t count, frames;

    // Output the wave data
    count = 0;
    do {
        frames = snd_pcm_writei (play->pcm, play->soundData->data + count, play->soundData->size - count);
        printf ("writen %ld frames of %ld left\n", frames, play->soundData->size - count);

        // If an error, try to recover from it
        if (frames < 0)
            frames = snd_pcm_recover(play->pcm, frames, 0);
        if (frames < 0) {
            printf("Error playing wave: %s\n", snd_strerror(frames));
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

    // Open audio card we wish to use for playback
    err = snd_pcm_open (&(play.pcm), &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
        selfLogErr ("Can't open audio %s: %s\n", &SoundCardPortName[0], snd_strerror (err));
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
            selfLogErr ("Can't set sound parameters: %s\n", snd_strerror(err));
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
