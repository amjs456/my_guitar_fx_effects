#ifndef TUNING_H
#define TUNING_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    const char *name;
    int midi[6];
} Tuning;

typedef struct {
    float reference_frequency_hz;
    const char *tuning;
    QueueHandle_t queue_handle;
} tuner_arg_t;

typedef struct {
    int target_midi_note;
    float diff_cent;
} TuningResult;

#endif