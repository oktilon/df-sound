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

// Include the ALSA .H file that defines ALSA functions/data
#include <alsa/asoundlib.h>

#include "sound.h"

typedef unsigned short      WORD;
typedef unsigned int        DWORD;
typedef unsigned char       BYTE;
typedef DWORD               FOURCC;


#pragma pack (1)
/////////////////////// WAVE File Stuff /////////////////////

// An IFF file header looks like this
typedef struct _FILE_head {
    FOURCC          ID;
    unsigned int    Length; // Length of subsequent file (including remainder of header). This is in
                                    // Intel reverse byte order if RIFF, Motorola format if FORM.
    FOURCC Type;    // {'W', 'A', 'V', 'E'} or {'A', 'I', 'F', 'F'}
} FILE_head;

typedef union _FOURCC_EXT {
    DWORD   fcc;
    struct {
        WORD wl;
        WORD wh;
    };
    BYTE    b[4];
} FOURCC_EXT;



// An IFF chunk header looks like this
typedef struct _CHUNK_head {
    FOURCC          ID;     // 4 ascii chars that is the chunk ID
    unsigned int    Length; // Length of subsequent data within this chunk. This is in Intel reverse byte
                            // order if RIFF, Motorola format if FORM. Note: this doesn't include any
                            // extra byte needed to pad the chunk out to an even size.
} CHUNK_head;

// WAVE fmt chunk
typedef struct _FORMAT {
    short           wFormatTag;         // 2
    unsigned short  wChannels;          // 2    4
    unsigned int    dwSamplesPerSec;    // 4    8
    unsigned int    dwAvgBytesPerSec;   // 4    12
    unsigned short  wBlockAlign;        // 2    14
    unsigned short  wBitsPerSample;     // 2    16
    unsigned short  cbSize;             // 2    18
  // Note: there may be additional fields here, depending upon wFormatTag
} FORMAT;

typedef struct broadcast_audio_extension {
    char Description[256]; /* ASCII : «Description of the sound sequence» */
    char Originator[32]; /* ASCII : «Name of the originator» */
    char OriginatorReference[32]; /* ASCII : «Reference of the originator» */
    char OriginationDate[10]; /* ASCII : «yyyy-mm-dd» */
    char OriginationTime[8]; /* ASCII : «hh-mm-ss» */
    DWORD TimeReferenceLow; /* First sample count since midnight low word */
    DWORD TimeReferenceHigh; /* First sample count since midnight, high word */
    WORD Version; /* Version of the BWF; unsigned binary number */
    BYTE UMID[64];
    BYTE Reserved[190] ; /* 190 bytes, reserved for future use, set to “NULL” */
    char CodingHistory[]; /* ASCII : « History coding » */
} BROADCAST_EXT;

typedef struct _CuePoint {
    DWORD   ID;
    DWORD   Position;
    DWORD   DataChunkID;
    DWORD   ChunkStart;
    DWORD   BlockStart;
    DWORD   SampleOffset;
} CuePoint;

#pragma pack()






// Size of the audio card hardware buffer. Here we want it
// set to 1024 16-bit sample points. This is relatively
// small in order to minimize latency. If you have trouble
// with underruns, you may need to increase this, and PERIODSIZE
// (trading off lower latency for more stability)
#define BUFFERSIZE  (2*1024)

// How many sample points the ALSA card plays before it calls
// our callback to fill some more of the audio card's hardware
// buffer. Here we want ALSA to call our callback after every
// 64 sample points have been played
#define PERIODSIZE  (2*64)

// Handle to ALSA (audio card's) playback port
snd_pcm_t               *PlaybackHandle;

// Handle to our callback thread
snd_async_handler_t *CallbackHandle;

// Points to loaded WAVE file's data
unsigned char           *WavePtr;

// Size (in frames) of loaded WAVE file's data
snd_pcm_uframes_t       WaveSize;

// Sample rate
unsigned short          WaveRate;

// Bit resolution
unsigned char           WaveBits;

// Block align
unsigned char           WaveAlign;

// Number of channels in the wave file
unsigned char           WaveChannels;

// The name of the ALSA port we output to. In this case, we're
// directly writing to hardware card 0,0 (ie, first set of audio
// outputs on the first audio card)
static const char       SoundCardPortName[] = "default";

static const unsigned int uRiff = 0x46464952;   // RIFF
static const unsigned int uWave = 0x45564157;   // WAVE
static const unsigned int uFmt  = 0x20746D66;   // fmt_
static const unsigned int uData = 0x61746164;   // data
static const unsigned int uBext = 0x74786562;   // bext
static const unsigned int uIxml = 0x4c4d5869;   // iXml
static const unsigned int uList = 0x5453494C;   // LIST
static const unsigned int uCue  = 0x20657563;   // cue_

const char * fourcc_txt (FOURCC fcc) {
    static char buf[16];
    memset (buf, 0, 16);
    sprintf (buf, "0x%08X", fcc);
    return buf;
}

const char * hex_dump (BYTE *data, BYTE sz) {
    static char buf[512];
    BYTE i;
    char del[2] = { 0, 0 };

    memset (buf, 0, 512);
    for (i = 0; i < sz; i++) {
        sprintf (buf + (i*3), "%s%02X", del, data[i]);
        if (i) del[0] = ' ';
    }
    return buf;
}

const char * fixed_to_sz (char *data, size_t sz) {
    static char buf[512];
    size_t max = sz > 511 ? 511 : sz;

    memset (buf, 0, 512);
    strncpy (buf, data, max);
    return buf;
}

const char * fourcc_char (FOURCC fcc) {
    return fixed_to_sz ((char *)(&fcc), 4);
}

const char * fourcc_sz (FOURCC fcc) {
    static char buf[5];
    FOURCC_EXT fcce;
    int i;

    memset (buf, 0, 5);
    fcce.fcc = fcc;
    for (i = 0; i < 4; i++) {
        buf[i] = fcce.b[i] > 31 ? fcce.b[i] : '.';
    }
    return buf;
}


/********************** waveLoad() *********************
 * Loads a WAVE file.
 *
 * fn =         Filename to load.
 *
 * RETURNS: 0 if success, non-zero if not.
 *
 * NOTE: Sets the global "WavePtr" to an allocated buffer
 * containing the wave data, and "WaveSize" to the size
 * in sample points.
 */

static unsigned char waveLoad(const char *fn)
{
    const char     *message;
    FILE_head       head;
    register int    inHandle;
    long            fsz = 0;
    BYTE            ret = 1;

    if ((inHandle = open(fn, O_RDONLY)) == -1)
        message = "didn't open";

    // Read in IFF File header
    else
    {
        fsz = lseek(inHandle, 0L, SEEK_END);
        lseek(inHandle, 0L, SEEK_SET);

        printf ("File %s size=%ld\n", fn, fsz);

        if (read(inHandle, &head, sizeof(FILE_head)) == sizeof(FILE_head))
        {
            // Is it a RIFF and WAVE?
            if (uRiff != head.ID || uWave != head.Type)
            {
                message = "is not a WAVE file";
                goto bad;
            }

            printf ("File hdr: ID=%s [%s], Type=%s [%s], Len=%d\n", fourcc_char (head.ID), fourcc_txt (head.ID), fourcc_char (head.Type), fourcc_txt (head.Type), head.Length);

            // Read in next chunk header
            while (read(inHandle, &head, sizeof(CHUNK_head)) == sizeof(CHUNK_head)) {

                printf ("Chunk hdr: ID=%s [%s], Len=%d : ", fourcc_char (head.ID), fourcc_txt (head.ID), head.Length);
                // ============================ Is it a fmt chunk? ===============================
                if (uFmt == head.ID) {
                    FORMAT  format;

                    // Read in the remainder of chunk
                    if (read(inHandle, &format, head.Length) != head.Length) break;

                    // Can't handle compressed WAVE files
                    if (format.wFormatTag != WAVE_FORMAT_PCM)
                    {
                        message = "only PCM WAVE format is supported";
                        goto bad;
                    }

                    WaveBits = (unsigned char)format.wBitsPerSample;
                    WaveRate = (unsigned short)format.dwSamplesPerSec;
                    WaveAlign = (unsigned char)format.wBlockAlign;
                    WaveChannels = format.wChannels;

                    if (head.Length <= 14) {
                        printf ("FORMAT_PCM [ch=%d, samPerSec=%u, AvgBytesPerSec=%u, BlkAlgn=%u]\n"
                            , format.wChannels
                            , format.dwSamplesPerSec
                            , format.dwAvgBytesPerSec
                            , format.wBlockAlign);
                    } else if (head.Length == 16) {
                        printf ("FORMAT_PCM [ch=%d, samPerSec=%u, AvgBytesPerSec=%u, BlkAlgn=%u, BitsPerSamp=%u]\n"
                            , format.wChannels
                            , format.dwSamplesPerSec
                            , format.dwAvgBytesPerSec
                            , format.wBlockAlign
                            , format.wBitsPerSample);
                    } else if (head.Length >= 18) {
                        printf ("FORMAT_PCM [ch=%d, samPerSec=%u, AvgBytesPerSec=%u, BlkAlgn=%u, BitsPerSamp=%u, cbSize=%u]\n"
                            , format.wChannels
                            , format.dwSamplesPerSec
                            , format.dwAvgBytesPerSec
                            , format.wBlockAlign
                            , format.wBitsPerSample
                            , format.cbSize);
                    }
                }

                // ============================ Is it a data chunk? ===============================
                else if (uData == head.ID) {
                    ssize_t sz;
                    // Size of wave data is head.Length. Allocate a buffer and read in the wave data
                    if (!(WavePtr = (unsigned char *)malloc(head.Length)))
                    {
                        message = "won't fit in RAM";
                        goto bad;
                    }
                    sz = read(inHandle, WavePtr, head.Length);
                    if (sz != head.Length)
                    {
                        free(WavePtr);
                        printf ("Data chunk sz=%ld not equal head.Length=%u\n", sz, head.Length);
                        break;
                    }

                    // Store size (in frames)
                    WaveSize = (head.Length * 8) / ((unsigned int)WaveBits * (unsigned int)WaveChannels);

                    printf ("DATA sz=%ld, WaveSize=%ld\n", sz, WaveSize);

                    ret = 0;
                }

                // ============================ Is it a bext chunk? ===============================
                else if (uBext == head.ID) {
                    ssize_t sz;
                    BROADCAST_EXT *bExt = NULL;
                    // Size of wave data is head.Length. Allocate a buffer and read in the wave data
                    if (!(bExt = (BROADCAST_EXT *)malloc(head.Length))) {
                        message = "won't fit in RAM";
                        goto bad;
                    }
                    sz = read(inHandle, bExt, head.Length);
                    if (sz != head.Length) {
                        free(bExt);
                        printf ("bext chunk sz=%ld not equal head.Length=%u\n", sz, head.Length);
                        break;
                    }

                    printf ("\n");
                    printf ("   Description  : %s\n", fixed_to_sz (bExt->Description, 256));
                    printf ("   Originator   : %s\n", fixed_to_sz (bExt->Originator, 32));
                    printf ("    - Reference : %s\n", fixed_to_sz (bExt->OriginatorReference, 32));
                    printf ("    - Date      : %s\n", fixed_to_sz (bExt->OriginationDate, 10));
                    printf ("    - Time      : %s\n", fixed_to_sz (bExt->OriginationTime, 8));
                    printf ("   TimeRefLow   : 0x%X\n", bExt->TimeReferenceLow);
                    printf ("   TimeRefHigh  : 0x%X\n", bExt->TimeReferenceHigh);
                    printf ("   Version      : 0x%X\n", bExt->Version);
                    printf ("   SMPTE UMID   : %s\n", hex_dump (bExt->UMID, 64));
                    printf ("   History      : %s\n\n", bExt->CodingHistory);
                    free(bExt);
                }

                // ============================ iXML chunk (dump it) ===============================
                else if (uIxml == head.ID) {
                    ssize_t sz;
                    char *xml = NULL;
                    // Size of wave data is head.Length. Allocate a buffer and read in the wave data
                    if (!(xml = (char *)malloc(head.Length))) {
                        message = "won't fit in RAM";
                        goto bad;
                    }
                    memset (xml, 0, head.Length);
                    sz = read(inHandle, xml, head.Length);
                    if (sz != head.Length) {
                        free(xml);
                        printf ("iXML chunk sz=%ld not equal head.Length=%u\n", sz, head.Length);
                        break;
                    }

                    printf ("\n   %s\n\n", xml);
                    free (xml);
                }

                // ============================ cue chunk (dump it) ===============================
                else if (uCue == head.ID) {
                    ssize_t sz;
                    DWORD i, cueCount = 0;
                    WORD len = head.Length;
                    CuePoint *cues = NULL;

                    if (len < 4) {
                        printf(" wrong cue size. Skip it\n");
                        if (len & 1) ++len;  // If odd, round it up to account for pad byte
                        lseek(inHandle, len, SEEK_CUR);
                    } else {
                        sz = read(inHandle, &cueCount, 4);
                        if (sz != 4) {
                            printf ("error read cue count");
                            break;
                        }
                        printf (" cue count :%d\n", cueCount);
                        len -= 4;
                        if (!(cues = (CuePoint *)malloc(len))) {
                            message = "won't fit in RAM";
                            goto bad;
                        }
                        memset (cues, 0, len);
                        sz = read(inHandle, cues, len);
                        if (sz != len) {
                            free(cues);
                            printf ("cue chunk sz=%ld not equal head.Length=%u\n", sz, len);
                            break;
                        }

                        for (i = 0; i < cueCount; i++) {
                            printf ("   cue id:%d, pos:%d, chunk:0x%X\n", cues[i].ID, cues[i].Position, cues[i].DataChunkID);
                        }

                        free (cues);
                    }
                }

                // ============================ LIST chunk (dump it) ===============================
                else if (uList == head.ID) {
                    ssize_t sz;
                    FOURCC listType;
                    WORD len = head.Length;
                    CHUNK_head item;
                    char *data = NULL;
                    DWORD szHdr = sizeof(CHUNK_head);
                    DWORD szData;

                    if (len < 4) {
                        printf(" wrong list size. Skip it\n");
                        if (len & 1) ++len;  // If odd, round it up to account for pad byte
                        lseek(inHandle, len, SEEK_CUR);
                    } else {
                        sz = read(inHandle, &listType, 4);
                        if (sz != 4) {
                            printf ("error read list type");
                            break;
                        }
                        printf (" type=%s [0x%08X]\n", fourcc_char (listType), listType);
                        len -= 4;
                        while (len > 0) {
                            memset (&item, 0, szHdr);
                            sz = read(inHandle, &item, szHdr);
                            if (sz != szHdr) {
                                printf ("list item hdr sz=%ld not equal %d\n", sz, szHdr);
                                break;
                            }
                            len -= szHdr;
                            szData = item.Length;

                            printf ("   item id:0x%08X [%s, len=%d", item.ID, fourcc_sz (item.ID), item.Length);
                            if (len < szData) {
                                printf (" (cut %d) ", len);
                                szData = len;
                            }
                            printf ("] = ");

                            data = (char *) malloc (szData);
                            if (!data) {
                                message = "won't fit in RAM";
                                goto bad;
                            }
                            sz = read(inHandle, data, szData);
                            if (sz != szData) {
                                printf ("list item data sz=%ld not equal szData=%u\n", sz, szData);
                                break;
                            }
                            len -= szData;

                            if (len & 1) {
                                --len;
                                lseek(inHandle, 1, SEEK_CUR);
                            }

                            printf ("%s\n", data);

                            free (data);
                        }
                    }
                }

                // ============================ Skip this chunk ===============================
                else {
                    printf ("SKIP sz=%d\n", head.Length);
                    if (head.Length & 1) head.Length++;
                    lseek(inHandle, head.Length, SEEK_CUR);
                }
            }
        }

        if (ret) {
            message = "is a bad WAVE file";
        }
bad:    close(inHandle);
    }

    printf("%s %s\n", fn, message);
    return ret;
}









/********************** play_audio() **********************
 * Plays the loaded waveform.
 *
 * NOTE: ALSA sound card's handle must be in the global
 * "PlaybackHandle". A pointer to the wave data must be in
 * the global "WavePtr", and its size of "WaveSize".
 */

static void play_audio(void)
{
    register snd_pcm_uframes_t      count, frames;

    // Output the wave data
    count = 0;
    do
    {
        frames = snd_pcm_writei(PlaybackHandle, WavePtr + count, WaveSize - count);
        printf ("writen %ld frames of %ld left\n", frames, WaveSize - count);

        // If an error, try to recover from it
        if (frames < 0)
            frames = snd_pcm_recover(PlaybackHandle, frames, 0);
        if (frames < 0)
        {
            printf("Error playing wave: %s\n", snd_strerror(frames));
            break;
        }

        // Update our pointer
        count += frames;

    } while (count < WaveSize);

    // Wait for playback to completely finish
    //if (count == WaveSize)
        //snd_pcm_drain(PlaybackHandle);
}





/*********************** free_wave_data() *********************
 * Frees any wave data we loaded.
 *
 * NOTE: A pointer to the wave data be in the global
 * "WavePtr".
 */

static void free_wave_data(void)
{
    if (WavePtr) free(WavePtr);
    WavePtr = 0;
}

void *playwave(void* data)
{
    // No wave data loaded yet
    WavePtr = 0;
    char* filepath = (char*) data;
    uint8_t align3 = 0;

    // Load the wave file
    if (!waveLoad(filepath))
    {
        register int        err;

        // Open audio card we wish to use for playback
        if ((err = snd_pcm_open(&PlaybackHandle, &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0)) < 0)
            printf("Can't open audio %s: %s\n", &SoundCardPortName[0], snd_strerror(err));
        else
        {
            if ((WaveAlign % 3) == 0)
                align3 = 1;

            switch (WaveBits) {
                case 8:
                    err = SND_PCM_FORMAT_U8;
                    printf ("play: Unsigned 8 bit\n");
                    break;

                case 16:
                    err = SND_PCM_FORMAT_S16_LE;
                    printf ("play: Signed 16 bit Little Endian\n");
                    break;

                case 24:
                    if (align3) {
                        err = SND_PCM_FORMAT_S24_3LE;
                        printf ("play: Signed 24 bit Little Endian in 3bytes\n");
                    } else {
                        err = SND_PCM_FORMAT_S24;
                        printf ("play: Signed 24 bit Little Endian\n");
                    }
                    break;

                case 32:
                    err = SND_PCM_FORMAT_S32_LE;
                    printf ("play: Signed 32 bit Little Endian\n");
                    break;
            }

            // Set the audio card's hardware parameters (sample rate, bit resolution, etc)
            //                                           Format         Access                  Channels      Rate    SoftResample Latency
            if ((err = snd_pcm_set_params(PlaybackHandle, err, SND_PCM_ACCESS_RW_INTERLEAVED, WaveChannels, WaveRate,      1,      100000)) < 0)
                printf("Can't set sound parameters: %s\n", snd_strerror(err));

            // Play the waveform
            else
                play_audio();

        }
    }

    pthread_exit(NULL);
}
int main(int argc, char **argv)
{
    // Create 2 threads
    pthread_t thread_id1;
    pthread_t thread_id2;
    pthread_t *wait = &thread_id2;

    // Run Playback Function in Separate Threads
    // printf ("argc=%d\n", argc);
    if (argc > 2) {
        pthread_create(&thread_id1, NULL, playwave, (void*)argv[2]);
        wait = &thread_id1;
        sleep(2);
    }
    pthread_create(&thread_id2, NULL, playwave, (void*)argv[1]);

    // Await 2nd threads return
    pthread_join(*wait, NULL);

    // Close Sound Card
    snd_pcm_drain(PlaybackHandle);
    snd_pcm_close(PlaybackHandle);
    free_wave_data();
    return(0);
}