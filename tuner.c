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

void fft_task(void *args){
    uint32_t selected_fft_input;
    float *fft_input = (float *)args;
    while(1){
        
        xTaskNotifyWait(0, 0, &selected_fft_input, portMAX_DELAY);
        //ここでfft
    }
}

float fft_input[2][FFT_SIZE * 2];

float tuner(float data){
    static bool initialized = false;
    static float window[FFT_SIZE];
    if(!initialized){
        BaseType_t ret = xTaskCreate(fft_task, "fft_task", 4906, &fft_input[0], 5, &fft_task_handle);
        if(ret != pdPASS){
            ESP_LOGE(TAG, "fft_task creation failef");
        }
        ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, FFT_SIZE));
        dsps_wind_hann_f32(window, FFT_SIZE);
        initialized = true;
    }

    static int count = 0;
    static float samples[FFT_SIZE];
    static uint32_t selected_fft_input = 0;

    if(count >= FFT_SIZE){
        for(int i = 0; i < FFT_SIZE; i++){
            fft_input[selected_fft_input][i * 2] = samples[i] * window [i];
            fft_input[selected_fft_input][i * 2 + 1] = 0.0f;
        }

        xTaskNotify(fft_task_handle, selected_fft_input, eSetValueWithOverwrite);
        

        memmove(samples, samples + HOP_SIZE, (FFT_SIZE - HOP_SIZE) * sizeof(float));

        count = FFT_SIZE - HOP_SIZE;
        selected_fft_input = !selected_fft_input;
    }
    samples[count] = data;
    count++;

    return data;
}