#pragma once

#include "app.h"

typedef enum DownloadStateEnum {
    DL_Init,
    DL_Process,
    DL_Finished
} DownloadState;

typedef struct DownloadDataStruct {
    uint64_t id;
    SoundType type;
    char url[MAX_URL_SIZE + 1];
    char filename[MAX_FILE_SIZE + 1];
    DownloadState state;
} DownloadData;

typedef struct ChunkDataStruct {
    uint32_t cnt;
    size_t size;
    FILE *fd;
} ChunkData;

int download_sound (SoundData *data);
int download_count ();