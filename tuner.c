#include "tuner.h"

#include "dsps_fft2r.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_dsp.h"
#include "portmacro.h"

#define FFT_SIZE 4096
#define FFT_LEN (FFT_SIZE * 2)
#define SAMPLE_RATE 48000.0f

static const char *TAG = "esp_tuner";
static TaskHandle_t fft_task_handle;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static float autocorr[FFT_SIZE];
static int min_lag = 130;
static int max_lag = 600;

typedef enum {
    BUFFER_STATE_FREE,
    BUFFER_STATE_WRITING,
    BUFFER_STATE_READY,
    BUFFER_STATE_PROCESSING
} BufferState;


typedef struct {
    float sample_buf[2][FFT_SIZE];
    BufferState sample_buf_state[2];
    tuner_arg_t *tuner_arg;
} fft_task_args_t;

const Tuning tunings[] = {
    {
        "Standard",
        {40, 45, 50, 55, 59, 64
        }
    },
};


static esp_err_t autocorrelation_fft(fft_task_args_t *fft_task_args, float fft_buf[FFT_LEN], int buffer_index, float mean){
    memset(fft_buf, 0, sizeof(&fft_buf) * FFT_LEN * 2);

    for(int j = 0; j < FFT_SIZE; j++){
        fft_buf[j * 2] = (fft_task_args->sample_buf[buffer_index][j] - mean);
    }

    ESP_ERROR_CHECK(dsps_fft2r_fc32(fft_buf, FFT_LEN));
    ESP_ERROR_CHECK(dsps_bit_rev_fc32(fft_buf, FFT_LEN));

    for(int j = 0; j < FFT_LEN; j++){
        float re = fft_buf[2 * j];
        float im = fft_buf[2 * j + 1];
        fft_buf[2 * j] = re * re + im * im;
        fft_buf[2 * j + 1] = 0.0f;
    }

    ESP_ERROR_CHECK(dsps_fft2r_fc32(fft_buf, FFT_LEN));
    ESP_ERROR_CHECK(dsps_bit_rev_fc32(fft_buf, FFT_LEN));

    for(int j = 0; j < FFT_SIZE; j++){
        autocorr[j] = fft_buf[2 * j] / FFT_LEN;
    }
    return ESP_OK;
}


static esp_err_t find_autocorr_peak_lag_and_max_value(int *peak_lag, float *max_value){
    *max_value = autocorr[min_lag];
    *peak_lag = min_lag;
  
    for(int j = min_lag + 1; j <= max_lag; j++){
        if(autocorr[j] > *max_value){
            *max_value = autocorr[j];
            *peak_lag = j;
        }
    }
    return ESP_OK;
}



void fft_task(void *args){
    fft_task_args_t *fft_task_args = (fft_task_args_t *)args;

    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, FFT_LEN));

    static float fft_buf[FFT_LEN * 2] __attribute__((aligned(16)));

    while(1){
        int index = -1;

        portENTER_CRITICAL(&mux);
        for(int i = 0; i < 2; i++){
            if(fft_task_args->sample_buf_state[i] == BUFFER_STATE_READY){
                fft_task_args->sample_buf_state[i] = BUFFER_STATE_PROCESSING;
                index = i;
                break;
            }
        }

        portEXIT_CRITICAL(&mux);


        if(index == -1){
            vTaskDelay(1);
            continue;
        }

        float mean = 0.0f;

        for (int j = 0; j <FFT_SIZE; j++){
            mean += fft_task_args->sample_buf[index][j];
        }

        mean /= FFT_SIZE;

        float rms = 0.0f;
        
        for(int j = 0; j <FFT_SIZE; j++){
            float x = fft_task_args->sample_buf[index][j] - mean;
            rms += x * x;
        }

        rms = sqrtf(rms / FFT_SIZE);
        
        if(rms > 0.005f){
            ESP_ERROR_CHECK(autocorrelation_fft(fft_task_args, fft_buf, index, mean));
            
            int peak_lag = -1;
            float max_value = NAN;
            ESP_ERROR_CHECK(find_autocorr_peak_lag_and_max_value(&peak_lag, &max_value));


            float confidence = max_value / autocorr[0];

            if(confidence > 0.5f){
                
                float frequency = SAMPLE_RATE / peak_lag;
                int detected_midi_note = (int)lround(69.0 + 12.0 * log2(frequency / fft_task_args->tuner_arg->reference_frequency_hz));

                int tunings_index = -1;
                size_t tuning_count = sizeof(tunings) / sizeof(tunings[0]);
                for(size_t k = 0; k < tuning_count; k++){
                    if(strcmp(tunings[k].name, fft_task_args->tuner_arg->tuning) == 0){
                        tunings_index = (int)k;
                        break;
                    }
                }
                
                int target_midi_note = -1;
                if(tunings_index != -1){
                    float min_diff = 2.0f;
                    
                    for(int k = 0; k < 6; k++){
                        float note_diff = fabsf((float)(detected_midi_note - tunings[tunings_index].midi[k]));
                        if(note_diff <= min_diff){
                            min_diff = note_diff;
                            target_midi_note = tunings[tunings_index].midi[k];
                        }
                    }
                }

                if(target_midi_note != -1){
                    float target_frequency = fft_task_args->tuner_arg->reference_frequency_hz * (float)(pow(2.0, (target_midi_note - 69.0) / 12.0));
                    float diff_cent = 1200.0 * (float)log2(frequency / target_frequency);
                    TuningResult tuning_result = {
                        .target_midi_note = target_midi_note,
                        .diff_cent = diff_cent
                    };
                    xQueueOverwrite(fft_task_args->tuner_arg->queue_handle, &tuning_result);
                }
            }
        }

        portENTER_CRITICAL(&mux);
        fft_task_args->sample_buf_state[index] = BUFFER_STATE_FREE;
        portEXIT_CRITICAL(&mux);   
        vTaskDelay(1);
    }
}

float tuner(float data, void *args){
    tuner_arg_t *tuner_arg = (tuner_arg_t *)args;
    static bool initialized = false;

    static fft_task_args_t fft_task_args = {
        .sample_buf_state = {BUFFER_STATE_FREE, BUFFER_STATE_FREE},
    };
    fft_task_args.tuner_arg = tuner_arg;

    if(!initialized){
        BaseType_t ret = xTaskCreate(fft_task, "fft_task", 4906, &fft_task_args, 5, &fft_task_handle);
        if(ret == pdPASS){
            initialized = true;
        } else {
            ESP_LOGE(TAG, "fft_task creation failef");
        }
        
    }


    static int count = 0;
    static int i = -1;

    portENTER_CRITICAL(&mux);
    if(i == -1){
        for(int j = 0; j < 2; j++){
            if(fft_task_args.sample_buf_state[j] == BUFFER_STATE_FREE){
                i = j;
                count = 0;
                fft_task_args.sample_buf_state[i] = BUFFER_STATE_WRITING;
                break;
            }
        }
    }

    if(i != -1){
        fft_task_args.sample_buf[i][count++] = data;
        if(count == FFT_SIZE){
            fft_task_args.sample_buf_state[i] = BUFFER_STATE_READY;
            int next = !i;
            if(fft_task_args.sample_buf_state[next] == BUFFER_STATE_FREE){
                i = next;
                fft_task_args.sample_buf_state[i] = BUFFER_STATE_WRITING;
            } else {
                i = -1;
            }
            count = 0;
        }
    }
    portEXIT_CRITICAL(&mux);

    return data;
}