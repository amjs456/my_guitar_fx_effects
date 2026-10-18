#include "dsps_fft2r.h"
#include "dsps_wind_hann.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "my_guitar_fx_effects.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_dsp.h"
#include "portmacro.h"

#define FFT_SIZE 4096
#define HOP_SIZE 1024

static const char *TAG = "esp_tuner";
static TaskHandle_t fft_task_handle;

typedef enum {
    BUFFER_STATE_FREE,
    BUFFER_STATE_WRITING,
    BUFFER_STATE_READY,
    BUFFER_STATE_PROCESSING
} BufferState;


typedef struct {
    float *fft_input;
    BufferState *fft_input_buffer_state;
} fft_task_args_t;

void fft_task(void *args){
    fft_task_args_t *fft_task_args = (fft_task_args_t *)args;

    while(1){
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        for(int i = 0; i < 2; i++){
            if(fft_task_args->fft_input_buffer_state[i] == BUFFER_STATE_READY){
                //ここでfft
                fft_task_args->fft_input_buffer_state[i] = BUFFER_STATE_FREE;
                break;
            }
        }
    }
}

float fft_input[2][FFT_SIZE * 2];

float tuner(float data){
    static bool initialized = false;
    static float window[FFT_SIZE];

    static BufferState fft_input_buffer_state[2] = {BUFFER_STATE_FREE, BUFFER_STATE_FREE};

    static fft_task_args_t fft_task_args = {
        .fft_input = *fft_input,
        .fft_input_buffer_state = fft_input_buffer_state,
    };

    if(!initialized){
        BaseType_t ret = xTaskCreate(fft_task, "fft_task", 4906, &fft_task_args, 5, &fft_task_handle);
        if(ret != pdPASS){
            ESP_LOGE(TAG, "fft_task creation failef");
        }
        ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, FFT_SIZE));
        dsps_wind_hann_f32(window, FFT_SIZE);
        initialized = true;
    }

    static int count = 0;
    static float samples[FFT_SIZE];

    if(count >= FFT_SIZE){
        for(int i = 0; i < 2; i++){
            if(fft_input_buffer_state[i] == BUFFER_STATE_FREE){
                fft_input_buffer_state[i] = BUFFER_STATE_WRITING;
            
                for(int j = 0; j < FFT_SIZE; j++){
                    fft_input[i][j * 2] = samples[j] * window [j];
                    fft_input[i][j * 2 + 1] = 0.0f;
                }

                fft_input_buffer_state[i] = BUFFER_STATE_READY;
                xTaskNotify(fft_task_handle, 0, eNoAction);

                memmove(samples, samples + HOP_SIZE, (FFT_SIZE - HOP_SIZE) * sizeof(float));
                
                break;
            }
        }
        count = FFT_SIZE - HOP_SIZE;
    }

    samples[count] = data;
    count++;

    return data;
}