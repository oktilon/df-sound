#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "config_schema.h"

#define MAX_CONFIG_FILE_SIZE    255
#define CONFIG_FILE             "sound.yml"

static const char * config_get_file ();
static void config_read (const char **file);
static void config_free ();

static ConfigData        *cfg = NULL;

int config_read_data (SoundShort **sounds) {
    int i, cnt = 0;

    config_read (NULL);
    if (!cfg)
        return 0;

    cnt = cfg->sounds_count;
    gVolume = cfg->volume;
    if (!cnt)
        return 0;

    *sounds = (SoundShort *) calloc (sizeof(SoundShort), cfg->sounds_count);
    for (i = 0; i < cnt; i++) {
        (*sounds)[i].id = cfg->sounds[i].id;
        (*sounds)[i].type = (SoundType) cfg->sounds[i].type;
        strncpy ((*sounds)[i].url, cfg->sounds[i].url, MAX_URL_SIZE);
    }

    // Free config struct
    config_free ();

    return cnt;
}

int config_write_data (SoundShort *sounds, int count) {
    // Variables
    uint8_t i, j, has;
    cyaml_err_t err = 0;
    // Log on error and exit
    const char *file;

    // Read config from file
    config_read (&file);
    if (!cfg)
        return FALSE;

    // Update sounds
    if (count && sounds) {
        for (j = 0; j < count; j++) {
            has = FALSE;
            for (i = 0; i < cfg->sounds_count; i++) {
                if (sounds[j].type == cfg->sounds[i].type) {
                    cfg->sounds[i].id  = sounds[j].id;
                    cfg->sounds[i].url = strdup (sounds[j].url);
                    has = TRUE;
                }
            }
            if (!has) { // Add new sound type config
                // Reallocate memory for config structure
                cfg->sounds = (SoundConfig *) realloc (cfg->sounds
                    , (sizeof (SoundConfig) * (cfg->sounds_count + 1))
                );

                if (cfg->sounds) { // On successful reallocation
                    // Fill structure
                    cfg->sounds[cfg->sounds_count].id   = sounds[j].id;
                    cfg->sounds[cfg->sounds_count].type = sounds[j].type;
                    cfg->sounds[cfg->sounds_count].url  = strdup (sounds[j].url);
                    // Increment counter
                    cfg->sounds_count++;
                } else { // On reallocation error
                    // Log error
                    selfLogErr ("Config reallocate error(%d): %m", errno);
                    // Break the loop
                    err = -errno;
                    break;
                }
            }
        }
    }

    if (!err) {

        // Update volume
        cfg->volume = gVolume;

        // Save updated config
        err = cyaml_save_file (file, &ymlConfig
            , &schemaCache, (cyaml_data_t *)cfg, 0);
        if (err != CYAML_OK) { // Log on error and exit
            selfLogWrn ("CYaml save file error(%d): %s", err, cyaml_strerror (err));
            return FALSE;
        }
        selfLogTrc ("Config updated successfully!");
    }

    // Free config struct
    config_free ();

    return err ? FALSE : TRUE;
}

void config_set_volume () {
    // Variable
    cyaml_err_t err;
    // Config file path
    const char *file = config_get_file ();

    // Read current config
    cfg = NULL;
    err = cyaml_load_file (file, &ymlConfig
        , &schemaCache, (cyaml_data_t **)&cfg, NULL);
    if (err != CYAML_OK) { // Log on error and exit
        selfLogWrn ("CYaml load file error(%d): %s", err, cyaml_strerror (err));
        return;
    }

    // Update only volume
    cfg->volume = gVolume;


    err = cyaml_save_file (file, &ymlConfig
        , &schemaCache, (cyaml_data_t *)cfg, 0);
    if (err != CYAML_OK) { // Log on error and exit
        selfLogWrn ("CYaml save file error(%d): %s", err, cyaml_strerror (err));
        return;
    }

    // Free config struct
    config_free ();
}

static const char * config_get_file () {
    static char path[MAX_CONFIG_FILE_SIZE + 1] = {0};

    if (path[0])
        return path;

    /// TODO Update config file location
    snprintf (path, MAX_CONFIG_FILE_SIZE, "%s/%s", getenv ("HOME"), CONFIG_FILE);
    return path;
}

static void config_read (const char **filename) {
    cyaml_err_t err;
    const char *file = config_get_file ();

    if (filename) {
        *filename = file;
    }

    if (cfg) {
        selfLogWrn ("CYaml data is not empty!");
        cfg = NULL;
    }

    err = cyaml_load_file (file, &ymlConfig
        , &schemaCache, (cyaml_data_t **)&cfg, NULL);
    if (err != CYAML_OK) {
        selfLogWrn ("CYaml load file error(%d): %s", err, cyaml_strerror (err));
    }
}

static void config_free () {
    cyaml_err_t err;

    if (!cfg)
        return;

    err = cyaml_free (&ymlConfig, &schemaCache, cfg, 0);
    if (err != CYAML_OK)
        selfLogWrn ("CYaml free error(%d): %s", err, cyaml_strerror (err));

    cfg = NULL;
}
