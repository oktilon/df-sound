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
#include "bus.h"
#include "sound.h"

/** @brief Sound card name */
char        card[64]  = "default";
int         gLogLevel = LOG_LEVEL_WARNING;    // Logging level
int         gYamlLog  = FALSE;

static int  gLogType  = LOG_TYPE_NORMAL;

// Long command line options
const struct option longOptions[] = {
    {"verbose",         no_argument,        0,  'v'},
    {"quiet",           no_argument,        0,  'q'},
    {"yaml",            no_argument,        0,  'y'},
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

const char * selfLogTimestamp () {
    // Static buffer
    static char buf[60] = {0};
    // Variables
    int r;
    size_t sz;
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

    return buf;
}

void selfLogOutput (const char *file, int line, const char *func, int lvl, const char *tms, const char *msg) {
    // Output log to stdout
    if (lvl <= gLogLevel) {
        if (lvl < 0) {
            printf ("%s: [---] %s %s\n", tms, func, msg);
        } else {
            switch (gLogType) {
                case LOG_TYPE_EXTENDED:
                    printf ("%s: [%s] %s %s%s\033[0m [%s:%d]\n", tms, logLevelHeaders[lvl], func, logLevelColor[lvl], msg, file, line);
                    break;

                default: // LOG_TYPE_NORMAL
                    printf ("%s: [%s] %s %s%s\033[0m\n", tms, logLevelHeaders[lvl], func, logLevelColor[lvl], msg);
                    break;
            }
        }
        fflush (stdout);
    }
}

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
    char *msg = NULL;
    const char *tms = selfLogTimestamp ();

    // Format log message
    va_list arglist;
    va_start (arglist, fmt);
    r = vasprintf (&msg, fmt, arglist);
    va_end (arglist);

    selfLogOutput (file, line, func, lvl, tms, msg);

    // Free formatted log buffer
    if (r && msg)
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

            case 'y': // yaml logs
                gYamlLog = TRUE;
                break;

            default:
                break;
        }
    }
}

int main (int argc, char **argv) {
    app_parse_arguments (argc, argv);

    selfLog ("Started v.%s LogLevel=%s", APP_VERSION, logLevelHeaders[gLogLevel]);

    int err = sound_start ();

    selfLogErr ("Stopped");
    return err;
}
