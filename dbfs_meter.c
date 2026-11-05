#include "dbfs_meter.h"
#include "freertos/idf_additions.h"

#include <math.h>
#include <stdio.h>
#define SAMPLE_SIZE 2400

float dbfs_meter(float data, void *args){
    dbfs_meter_args_t *dbfs_meter_args = (dbfs_meter_args_t *)args;

    static float sum = 0.0f;
    static float peak = 0.0f;
    static int count = 0;

    sum += data * data;

    float abs_data = fabsf(data);
    if(abs_data > peak){
        peak = abs_data;
    }
    count++;

    if(count >= SAMPLE_SIZE) {
        float rms = sqrtf(sum / (float)count);


        float rms_dbfs;
        if(rms > 0.0f){
            rms_dbfs = 20.0f * log10f(rms);
        } else {
            rms_dbfs = -120.0f;
        }

        float peak_dbsf;
        if(peak > 0.0f){
            peak_dbsf = 20.0f * log10f(peak);
        } else {
            peak_dbsf = -120.0f;
        }

        dbfs_result_t dbfs_result = {
            .rms_dbfs = rms_dbfs,
            .peak_dbfs = peak_dbsf,
        };
        xQueueOverwrite(dbfs_meter_args->queue_handle, &dbfs_result);

        sum = 0.0f;
        count = 0;
        peak = 0.0f;
    }
    return data;
}