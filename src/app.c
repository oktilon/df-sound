#define _GNU_SOURCE
#include <alsa/asoundlib.h>
// #include <glib/gi18n-lib.h>
#include <locale.h>
#include <stdio.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <getopt.h>
#include "app.h"
// #include "dbus.h"
// #include "view_manager.h"
// #include "download_manager.h"

#define ERROR_CONTEXT       10
#define ERROR_REGISTER      11
#define ERROR_LOOP          12
#define ERROR_DBUS          13

static int              gLogLevel   = LOG_LEVEL_WARNING;    // Logging level
static int              gLogType    = LOG_TYPE_NORMAL;

int play(uint rate, int channels, int seconds);

// Long command line options
const struct option longOptions[] = {
    {"verbose",         no_argument,        0,  'v'},
    {"quiet",           no_argument,        0,  'q'},
    {"extended-log",    no_argument,        0,  'x'}
};

const char *logLevelHeaders[] = {
    "\033[1;31mERR\033[0m",  // LOG_LEVEL_ERROR     // q = quiet
    "\033[1;91mWRN\033[0m",  // LOG_LEVEL_WARNING   //   = default
    "\033[1;37mINF\033[0m",  // LOG_LEVEL_INFO      // v = verbose
    "\033[1;36mDBG\033[0m",  // LOG_LEVEL_DEBUG     // vvv = verbose++
    "\033[1;33mTRC\033[0m"   // LOG_LEVEL_TRACE     // vv = verbose+
};

const char *logLevelColor[] = {
    "\033[0;31m",  // LOG_LEVEL_ERROR   #BC1B27
    "\033[0;91m",  // LOG_LEVEL_WARNING #F15E42
    "\033[0;37m",  // LOG_LEVEL_INFO    #D0CFCC
    "\033[0;36m",  // LOG_LEVEL_DEBUG   #2AA1B3
    "\033[0;33m"   // LOG_LEVEL_TRACE   #A2734C
};
/**
 * @brief Logging main body
 *
 * @param file current file
 * @param line current line
 * @param func current function
 * @param lvl debug (verbosity) level
 * @param fmt log format
 * @param argp log other arguments
 */
void selfLogFunction (const char *file, int line, const char *func, int lvl, const char* fmt, ...) {
    // Variables
    int r;
    size_t sz;
    // sqlite3_stmt *db_st = NULL;
    char buf[60] = {0};
    char msec[30] = {0};
    struct timeval tv;
    struct tm t = {0};

    // Create timestamp with ms
    gettimeofday (&tv, NULL);
    localtime_r (&tv.tv_sec, &t);
    r = snprintf (msec, 30, "%ld", tv.tv_usec / 1000);
    for (; r < 3; r++) strcat (msec, "0");

    sz = strftime (buf, sizeof (buf), "%F %T", &t); // %F => %Y-%m-%d,  %T => %H:%M:%S
    snprintf (buf + sz, 60, ".%s", msec);
    char *msg = NULL;

    // Format log message
    va_list arglist;
    va_start (arglist, fmt);
    r = vasprintf (&msg, fmt, arglist);
    va_end (arglist);

    // Output log to stdout
    if (lvl <= gLogLevel) {
        if (lvl < 0) {
            printf ("%s: [---] %s %s\n", buf, func, msg);
        } else {
            switch (gLogType) {
                case LOG_TYPE_EXTENDED:
                    printf ("%s: [%s] %s %s%s\033[0m [%s:%d]\n", buf, logLevelHeaders[lvl], func, logLevelColor[lvl], msg, file, line);
                    break;

                default: // LOG_TYPE_NORMAL
                    printf ("%s: [%s] %s %s%s\033[0m\n", buf, logLevelHeaders[lvl], func, logLevelColor[lvl], msg);
                    break;
            }
        }
        fflush (stdout);
    }

    // Free formatted log buffer
    free (msg);
}

/**
 * @brief Parse cmdline arguments
 *
 * @param argc args count
 * @param argv args list
 */
void app_parse_arguments (int argc, char **argv) {
    int i;

    while ((i = getopt_long (argc, argv, "vqx", longOptions, NULL)) != -1) {
        switch (i) {
            case 'v': // verbose
                gLogLevel++;
                if (gLogLevel > LOG_LEVEL_MAX) {
                    gLogLevel = LOG_LEVEL_MAX;
                }
                break;

            case 'q': // quiet
                gLogLevel = LOG_LEVEL_ERROR;
                break;

            case 'x': // extended-log
                gLogType = LOG_TYPE_EXTENDED;
                break;

            default:
                break;
        }
    }
}

int main (int argc, char **argv) {
    app_parse_arguments (argc, argv);

    selfLog ("Started v.%d.%d.%d LogLevel=%s", VERSION_MAJOR, VERSION_MINOR, VERSION_REV, logLevelHeaders[gLogLevel]);

    play (48000, 2, 4);

    selfLogErr ("Stopped");
    return EXIT_SUCCESS;
}

/*
 * Simple sound playback using ALSA API and libasound.
 *
 * Compile:
 * $ cc -o play sound_playback.c -lasound
 *
 * Usage:
 * $ ./play <sample_rate> <channels> <seconds> < <file>
 *
 * Examples:
 * $ ./play 44100 2 5 < /dev/urandom
 * $ ./play 22050 1 8 < /path/to/file.wav
 *
 * Copyright (C) 2009 Alessandro Ghedini <al3xbio@gmail.com>
 * --------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Alessandro Ghedini wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we
 * meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * --------------------------------------------------------------
 */


#define PCM_DEVICE "default"

int play(uint rate, int channels, int seconds) {
	unsigned int pcm, tmp;
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	char *buff;
	int buff_size, loops;

	/* Open the PCM device in playback mode */
	if ((pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
					SND_PCM_STREAM_PLAYBACK, 0)) < 0)
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
					PCM_DEVICE, snd_strerror(pcm));

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_hw_params_any(pcm_handle, params);

	/* Set parameters */
	if ((pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
					SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

	if ((pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
						SND_PCM_FORMAT_S24_BE)) < 0)
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	if ((pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0)
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

	if ((pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0)
		printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

	/* Write parameters */
	if ((pcm = snd_pcm_hw_params(pcm_handle, params)) < 0)
		printf("ERROR: Can't set hardware parameters. %s\n", snd_strerror(pcm));

	/* Resume information */
	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	printf("channels: %i ", tmp);

	if (tmp == 1)
		printf("(mono)\n");
	else if (tmp == 2)
		printf("(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	printf("rate: %d bps\n", tmp);

	printf("seconds: %d\n", seconds);

    int fd = open("10.wav", 0, O_RDONLY);
    if (!fd) {
        printf ("ERROR: Can't open sound file(%d): %m\n", errno);
    } else {

        lseek (fd, 44, SEEK_SET);

        /* Allocate buffer to hold single period */
        snd_pcm_hw_params_get_period_size(params, &frames, 0);

        buff_size = frames * channels * 2 /* 2 -> sample size */;
        buff = (char *) malloc(buff_size);

        snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

        for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {

            if ((pcm = read(fd, buff, buff_size)) == 0) {
                printf("Early end of file.\n");
                return 0;
            }

            if ((pcm = snd_pcm_writei(pcm_handle, buff, frames)) == -EPIPE) {
                printf("XRUN.\n");
                snd_pcm_prepare(pcm_handle);
            } else if (pcm < 0) {
                printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
            }

        }

        close (fd);
	    free(buff);
    }

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);

	return 0;
}