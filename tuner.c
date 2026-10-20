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

#define FFT_SIZE 8192

static const char *TAG = "esp_tuner";
static TaskHandle_t fft_task_handle;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

typedef enum {
    BUFFER_STATE_FREE,
    BUFFER_STATE_WRITING,
    BUFFER_STATE_READY,
    BUFFER_STATE_PROCESSING
} BufferState;


typedef struct {
    float sample_buf[2][FFT_SIZE];
    BufferState sample_buf_state[2];
} fft_task_args_t;

void fft_task(void *args){
    fft_task_args_t *fft_task_args = (fft_task_args_t *)args;

    static float window[FFT_SIZE];
    dsps_wind_hann_f32(window, FFT_SIZE);
    ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, FFT_SIZE));

    static float fft_buf[FFT_SIZE * 2];

    esp_err_t err;

    while(1){
        for(int i = 0; i < 2; i++){
            bool ready = false;

            portENTER_CRITICAL(&mux);
            if(fft_task_args->sample_buf_state[i] == BUFFER_STATE_READY){
                fft_task_args->sample_buf_state[i] = BUFFER_STATE_PROCESSING;
                ready = true;
            }
            portEXIT_CRITICAL(&mux);

            if(ready){
                for(int j = 0; j < FFT_SIZE; j++){
                    fft_buf[j * 2] = fft_task_args->sample_buf[i][j] * window[j];
                    fft_buf[j * 2 + 1] = 0.0f;
                }

                ESP_ERROR_CHECK(dsps_fft2r_fc32(fft_buf, FFT_SIZE));
                ESP_ERROR_CHECK(dsps_bit_rev_fc32(fft_buf, FFT_SIZE));

                static float power_buf[FFT_SIZE];
                for(int k = 0; k < FFT_SIZE; k++){
                    float re = fft_buf[2 * k];
                    float im = fft_buf[2 * k + 1];
                    float power = re * re + im * im;
                    power_buf[k] = power;
                }

                int max_index = 0;
                for (int l = 1; l<FFT_SIZE;l++){
                    if(power_buf[l] > power_buf[max_index]){
                        max_index = l;
                    }
                }
                printf("%f\n", power_buf[max_index]);
                printf("%d\n", max_index);

                portENTER_CRITICAL(&mux);
                fft_task_args->sample_buf_state[i] = BUFFER_STATE_FREE;
                portEXIT_CRITICAL(&mux);
                break;
            }
        }
        vTaskDelay(1);
    }
}

float tuner(float data){
    static bool initialized = false;

    static fft_task_args_t fft_task_args = {
        .sample_buf_state = {BUFFER_STATE_FREE, BUFFER_STATE_FREE},
    };

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