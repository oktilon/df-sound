#pragma once

#include "common.h"
#include "cyaml/cyaml.h"

/**
 * @brief Sound data stored in Yaml
 */
typedef struct SoundConfigStruct {
    SoundType   type;
    uint64_t    id;
    const char *url;
} SoundConfig;

/**
 * @brief Sound service Yaml config struct
 */
typedef struct ConfigDataStruct {
    uint8_t      volume;
    SoundConfig *sounds;
    uint8_t      sounds_count;
} ConfigData;

/**
 * @brief Sound types enum dictionary
 */
static const cyaml_strval_t soundTypeStrings[] = {
    { "open", SoundOpen },
    { "call", SoundCall },
};

/**
 * @brief Sound data fields schema
 * @param type - Sound type (enum: [open, call])
 * @param id   - Sound Id
 * @param url  - Sound url
 */
static const cyaml_schema_field_t schemaSoundFields[] = {
    CYAML_FIELD_ENUM       ("type", CYAML_FLAG_DEFAULT
        , SoundConfig, type, soundTypeStrings
        , CYAML_ARRAY_LEN (soundTypeStrings)),

    CYAML_FIELD_UINT       ("id",   CYAML_FLAG_STRICT
        , SoundConfig, id),

    CYAML_FIELD_STRING_PTR ("url",  CYAML_FLAG_POINTER
        , SoundConfig, url, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * @brief Sound data mapping schema
 */
static const cyaml_schema_value_t schemaSound = {
    CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT, SoundConfig, schemaSoundFields),
};


/**
 * @brief Sound service config fields schema
 * @param volume - volume level percent (0 - 100)
 * @param sounds - sound data array (sequence)
 */
static const cyaml_schema_field_t schemaCacheFields[] = {
    CYAML_FIELD_UINT     ("volume", CYAML_FLAG_STRICT, ConfigData, volume),

    CYAML_FIELD_SEQUENCE ("sounds", CYAML_FLAG_POINTER, ConfigData, sounds
        , &schemaSound, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * @brief Top Yaml mapping schema
 * @param
 */
static const cyaml_schema_value_t schemaCache = {
    CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER, ConfigData, schemaCacheFields),
};

/**
 * @brief CYaml config
 * @param log_fn - lib logging function (disable it)
 * @param mem_fn - lib memory allocation function
 */
static cyaml_config_t ymlConfig = {
    .log_fn = NULL,
    .mem_fn = cyaml_mem
};