#ifndef DBFS_METER_H
#define DBFS_METER_H

#include "freertos/idf_additions.h"
#include <stdint.h>

typedef struct {
    float rms_dbfs;
    float peak_dbfs;
} dbfs_result_t;

typedef struct {
    QueueHandle_t queue_handle;
} dbfs_meter_args_t;

#endif