#ifndef Common_H
#define Common_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <systemd/sd-bus.h>
#include <alsa/asoundlib.h>


#define DBUS_OK(r)          (r >= 0)
#define UNUSED_ARG(arg)     ((void)arg)

#define MAX_URL_SIZE        255
#define MAX_FILE_SIZE       63

#define TRUE    1
#define FALSE   0

extern char     card[64];
extern int      gLogLevel;
extern uint8_t  gVolume;

/*************************
 *  SERVICE ENUMERATIONS
 *************************/
typedef enum AppStateEnum {
      SND_Initializing
    , SND_Downloading
    , SND_Idle
    , SND_Playing
} AppState;

typedef enum SoundTypeEnum {
      SoundNone
    , SoundTest
    , SoundOpen
    , SoundCall
    , SoundMAX
} SoundType;

typedef struct SoundShortStruct {
    SoundType           type;
    uint64_t            id;
    char                url[MAX_URL_SIZE + 1];
} SoundShort;

typedef struct SoundDataStruct {
    SoundType           type;
    uint64_t            id;
    char                url[MAX_URL_SIZE + 1];
    char                filename[MAX_FILE_SIZE + 1];
    uint8_t            *data;   // Points to loaded WAVE file's data
    snd_pcm_format_t    format;
    snd_pcm_uframes_t   size;   // Size (in frames) of loaded WAVE file's data
    uint32_t            rate;   // Sample rate
    uint16_t            bits;   // Bit resolution
    uint16_t            align;  // Block align
    uint16_t            channels; // Number of channels in the wave file
} SoundData;


// Colors
#define CSI                 "\033["
#define SGR(n)              CSI n "m"
#define SGR_BOLD(n)         CSI "1;" n "m"

#define SGR_CODE_BLACK          "0"
#define SGR_CODE_RED            "1"
#define SGR_CODE_GREEN          "2"
#define SGR_CODE_YELLOW         "3"
#define SGR_CODE_BLUE           "4"
#define SGR_CODE_MAGENTA        "5"
#define SGR_CODE_CYAN           "6"
#define SGR_CODE_WHITE          "7"

#define SGR_FORE(clr)           "3" clr
#define SGR_BACK(clr)           "4" clr
#define SGR_FORE_BRIGHT(clr)    "9" clr
#define SGR_BACK_BRIGHT(clr)    "10" clr

#define COLOR_NONE          SGR("0")
#define COLOR_BLACK         SGR(SGR_FORE(SGR_CODE_BLACK))
#define COLOR_RED           SGR(SGR_FORE(SGR_CODE_RED))
#define COLOR_GREEN         SGR(SGR_FORE(SGR_CODE_GREEN))
#define COLOR_YELLOW        SGR(SGR_FORE(SGR_CODE_YELLOW))
#define COLOR_BLUE          SGR(SGR_FORE(SGR_CODE_BLUE))
#define COLOR_MAGENTA       SGR(SGR_FORE(SGR_CODE_MAGENTA))
#define COLOR_CYAN          SGR(SGR_FORE(SGR_CODE_CYAN))
#define COLOR_BR_WHITE      SGR(SGR_FORE_BRIGHT(SGR_CODE_WHITE))
#define COLOR_BR_BLACK      SGR(SGR_FORE_BRIGHT(SGR_CODE_BLACK))
#define COLOR_BR_RED        SGR(SGR_FORE_BRIGHT(SGR_CODE_RED))
#define COLOR_BR_GREEN      SGR(SGR_FORE_BRIGHT(SGR_CODE_GREEN))
#define COLOR_BR_YELLOW     SGR(SGR_FORE_BRIGHT(SGR_CODE_YELLOW))
#define COLOR_BR_BLUE       SGR(SGR_FORE_BRIGHT(SGR_CODE_BLUE))
#define COLOR_BR_MAGENTA    SGR(SGR_FORE_BRIGHT(SGR_CODE_MAGENTA))
#define COLOR_BR_CYAN       SGR(SGR_FORE_BRIGHT(SGR_CODE_CYAN))
#define COLOR_BR_WHITE      SGR(SGR_FORE_BRIGHT(SGR_CODE_WHITE))
#define BOLD_BLACK          SGR_BOLD(SGR_FORE(SGR_CODE_BLACK))
#define BOLD_RED            SGR_BOLD(SGR_FORE(SGR_CODE_RED))
#define BOLD_GREEN          SGR_BOLD(SGR_FORE(SGR_CODE_GREEN))
#define BOLD_YELLOW         SGR_BOLD(SGR_FORE(SGR_CODE_YELLOW))
#define BOLD_BLUE           SGR_BOLD(SGR_FORE(SGR_CODE_BLUE))
#define BOLD_MAGENTA        SGR_BOLD(SGR_FORE(SGR_CODE_MAGENTA))
#define BOLD_CYAN           SGR_BOLD(SGR_FORE(SGR_CODE_CYAN))
#define BOLD_WHITE          SGR_BOLD(SGR_FORE(SGR_CODE_WHITE))
#define BOLD_BR_BLACK       SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_BLACK))
#define BOLD_BR_RED         SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_RED))
#define BOLD_BR_GREEN       SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_GREEN))
#define BOLD_BR_YELLOW      SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_YELLOW))
#define BOLD_BR_BLUE        SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_BLUE))
#define BOLD_BR_MAGENTA     SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_MAGENTA))
#define BOLD_BR_CYAN        SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_CYAN))
#define BOLD_BR_WHITE       SGR_BOLD(SGR_FORE_BRIGHT(SGR_CODE_WHITE))


// UI service log levels
#define LOG_LEVEL_ALWAYS    -1
#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_WARNING   1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_DEBUG     3
#define LOG_LEVEL_TRACE     4
#define LOG_LEVEL_MAX       LOG_LEVEL_TRACE

#define LOG_TYPE_NORMAL     0
#define LOG_TYPE_EXTENDED   1

// Global functions
const char * selfLogTimestamp ();
void selfLogOutput (const char *file, int line, const char *func, int lvl, const char *tms, const char *msg);
void selfLogFunction (const char *file, int line, const char *func, int lvl, const char* fmt, ...);
// Help macros
#define LOG_FILENAME()      ((const char *)(__FILE__))
#define LOG_FUNCTION()      ((const char *)(__PRETTY_FUNCTION__))
// Log always
#define selfLog(FMT, ...)    selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_ALWAYS,  FMT __VA_OPT__ (,) __VA_ARGS__)
// Log ERROR level
#define selfLogErr(FMT, ...) selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_ERROR,   FMT __VA_OPT__ (,) __VA_ARGS__)
// Log WARNING level
#define selfLogWrn(FMT, ...) selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_WARNING, FMT __VA_OPT__ (,) __VA_ARGS__)
// Log INFO level
#define selfLogInf(FMT, ...) selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_INFO,    FMT __VA_OPT__ (,) __VA_ARGS__)
// Log DEBUG level
#define selfLogDbg(FMT, ...) selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_DEBUG,   FMT __VA_OPT__ (,) __VA_ARGS__)
// Log TRACE level
#define selfLogTrc(FMT, ...) selfLogFunction (LOG_FILENAME(), __LINE__, LOG_FUNCTION(), LOG_LEVEL_TRACE,   FMT __VA_OPT__ (,) __VA_ARGS__)

#define returnIfFailLevel(LVL, EXPR, FMT, ...) \
    do { \
        if ((EXPR)) \
            { } \
        else \
        { \
            selfLogFunction (LOG_FILENAME() \
                , __LINE__ \
                , LOG_FUNCTION() \
                , LVL \
                , FMT __VA_OPT__ (,) __VA_ARGS__); \
            return; \
        } \
    } while (0)

#define returnValIfFailLevel(LVL, EXPR, VAL, FMT, ...) \
    do { \
        if ((EXPR)) \
            { } \
        else \
        { \
            selfLogFunction (LOG_FILENAME() \
                , __LINE__ \
                , LOG_FUNCTION() \
                , LVL \
                , FMT __VA_OPT__ (,) __VA_ARGS__); \
            return (VAL); \
        } \
    } while (0)

#define returnIfFailErr(EXPR, FMT, ...)             returnIfFailLevel(LOG_LEVEL_ERROR,   (EXPR), FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnIfFailWrn(EXPR, FMT, ...)             returnIfFailLevel(LOG_LEVEL_WARNING, (EXPR), FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnIfFailInf(EXPR, FMT, ...)             returnIfFailLevel(LOG_LEVEL_INFO,    (EXPR), FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnIfFailDbg(EXPR, FMT, ...)             returnIfFailLevel(LOG_LEVEL_DEBUG,   (EXPR), FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnIfFailTrc(EXPR, FMT, ...)             returnIfFailLevel(LOG_LEVEL_TRACE,   (EXPR), FMT __VA_OPT__ (,) __VA_ARGS__)

#define returnValIfFailErr(EXPR, VAL, FMT, ...)     returnValIfFailLevel(LOG_LEVEL_ERROR,   (EXPR), VAL, FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnValIfFailWrn(EXPR, VAL, FMT, ...)     returnValIfFailLevel(LOG_LEVEL_WARNING, (EXPR), VAL, FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnValIfFailInf(EXPR, VAL, FMT, ...)     returnValIfFailLevel(LOG_LEVEL_INFO,    (EXPR), VAL, FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnValIfFailDbg(EXPR, VAL, FMT, ...)     returnValIfFailLevel(LOG_LEVEL_DEBUG,   (EXPR), VAL, FMT __VA_OPT__ (,) __VA_ARGS__)
#define returnValIfFailTrc(EXPR, VAL, FMT, ...)     returnValIfFailLevel(LOG_LEVEL_TRACE,   (EXPR), VAL, FMT __VA_OPT__ (,) __VA_ARGS__)

#define dbusReplyErrorOnFail(r, dbusMessage, msg, ...) \
    if (r < 0) { \
        selfLogErr (msg __VA_OPT__ (,) __VA_ARGS__); \
        return sd_bus_reply_method_errorf (dbusMessage, SD_BUS_ERROR_FAILED, msg __VA_OPT__ (,) __VA_ARGS__); \
    }

#define dbusOnFailErr(r, msg, ...) \
    if (r < 0) { \
        selfLogErr (msg __VA_OPT__ (,) __VA_ARGS__); \
        return r; \
    }

#define dbusReturnOnFailErr(r, ret, msg, ...) \
    if (r < 0) { \
        selfLogErr (msg __VA_OPT__ (,) __VA_ARGS__); \
        return ret; \
    }

#endif // Common_H