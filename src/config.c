#define _GNU_SOURCE
#include <stdio.h>

#include "config.h"
#include "config_schema.h"

#define MAX_CONFIG_FILE_SIZE    255
#define CONFIG_FILE             "sound.yml"

static const char * config_get_file ();

static struct ConfigData        *cfg = NULL;

int config_get_sounds (SoundShort **sounds) {
    cyaml_err_t err;
    uint32_t i;
    const char *file = config_get_file ();

    selfLogWrn ("Try to load: %s", file);

    cfg = NULL;
    ymlConfig.log_level = LOG_LEVEL_TRACE - gLogLevel;
    if (!gYamlLog)
        ymlConfig.log_fn = NULL;

    err = cyaml_load_file (file, &ymlConfig
        , &schemaCache, (cyaml_data_t **)&cfg, NULL);
    if (err != CYAML_OK) {
        selfLogWrn ("CYaml load file error(%d): %s", err, cyaml_strerror (err));
        return 0;
    }

    selfLogDbg ("Read config %p", cfg);

    selfLogWrn ("Volume: %d", cfg->volume);
    // selfLogWrn ("Sounds count=%d", cfg->sounds_count);
    // if (!cnt)
    //     return 0;

    // *sounds = (SoundShort *) calloc (sizeof(SoundShort), cfg->sounds_count);
    // for (i = 0; i < cfg->sounds_count; i++) {
        // selfLogTrc ("Read %d sound id=%d, url=%s", cfg->sounds[i].type, cfg->sounds[i].id, cfg->sounds[i].url);
        // sounds[i]->id = cfg->sounds[i].id;
        // sounds[i]->type = (SoundType) cfg->sounds[i].type;
        // strncpy (sounds[i]->url, cfg->sounds[i].url, MAX_URL_SIZE);

    // }

    // // Open
    // data[0].type = SoundOpen;
    // data[0].id = 4;
    // strcat (data[0].url, "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/4.wav");

    // // Call
    // data[1].type = SoundCall;
    // data[1].id = 10;
    // strcat (data[1].url, "https://defigo.s3.eu-central-1.amazonaws.com/doorbell_sounds/production/10.wav");

    // *sounds = data;

    return 0; // cfg->sounds_count;
}

int config_set_sounds (SoundShort *sounds, int count) {
    return TRUE;
}

void config_free () {
    cyaml_err_t err;

    if (!cfg)
        return;

    selfLogWrn ("Try to free yaml...");
    err = cyaml_free (&ymlConfig, &schemaCache, cfg, 0);
    if (err != CYAML_OK) {
        selfLogWrn ("CYaml free error(%d): %s", err, cyaml_strerror (err));
    }
}

void cyamlLogFunction (cyaml_log_t level, void *ctx, const char *fmt, va_list args) {
    // Variables
    int r;
    char *msg = NULL;
    const char *tms = selfLogTimestamp ();

    r = vasprintf (&msg, fmt, args);
    selfLogOutput ("cyaml", 0, "cyaml", LOG_LEVEL_TRACE - level, tms, msg);

    // Free formatted log buffer
    if (r && msg)
        free (msg);
}

static const char * config_get_file () {
    static char path[MAX_CONFIG_FILE_SIZE + 1] = {0};

    if (path[0])
        return path;

    snprintf (path, MAX_CONFIG_FILE_SIZE, "%s/%s", getenv ("HOME"), CONFIG_FILE);
    return path;
}