#pragma once

#include "common.h"
#include "cyaml/cyaml.h"

#define SOUND_LIST_COUNT_MIN        0
#define SOUND_LIST_COUNT_MAX        5

void cyamlLogFunction (cyaml_log_t level, void *ctx, const char *fmt, va_list args);

struct SoundConfig {
    enum SoundTypeEnum  type;
    uint64_t            id;
    const char         *url;
};

struct ConfigData {
    uint8_t             volume;
    struct SoundConfig *sounds;
    uint32_t            sounds_count;
};

static const cyaml_strval_t soundTypeStrings[] = {
    { "open", SoundOpen },
    { "call", SoundCall },
};

/**
 * @brief Sound data
 * @param type - Sound type (enum)
 * @param id   - Sound Id
 * @param url  - Sound url
 */
static const cyaml_schema_field_t schemaSoundFields[] = {
    CYAML_FIELD_ENUM       ("type", CYAML_FLAG_DEFAULT
        , struct SoundConfig, type, soundTypeStrings, CYAML_ARRAY_LEN (soundTypeStrings)),

    CYAML_FIELD_UINT       ("id",   CYAML_FLAG_STRICT
        , struct SoundConfig, id),

    CYAML_FIELD_STRING_PTR ("url",  CYAML_FLAG_POINTER
        , struct SoundConfig, url, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * @brief Sound mapping schema
 *
 */
static const cyaml_schema_value_t schemaSound = {
    CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT, struct SoundConfig, schemaSoundFields),
};


/**
 * @brief Cache data
 *
 */
static const cyaml_schema_field_t schemaCacheFields[] = {
    CYAML_FIELD_UINT     ("volume", CYAML_FLAG_STRICT, struct ConfigData, volume),

    CYAML_FIELD_SEQUENCE ("sounds", CYAML_FLAG_POINTER, struct ConfigData, sounds
        , &schemaSound, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * @brief Top Yaml mapping schema
 *
 */
static const cyaml_schema_value_t schemaCache = {
    CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER, struct ConfigData, schemaCacheFields),
};

//! .yml config
static cyaml_config_t ymlConfig = {
    .log_fn = NULL,
    // .log_fn = cyaml_log,
    // .log_fn = cyamlLogFunction
    .mem_fn = cyaml_mem,
    // .flags = CYAML_CFG_DOCUMENT_DELIM
};