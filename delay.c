#include "delay.h"

#include "dsps_biquad.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "string.h"
#include "dsps_biquad_gen.h"

static char *TAG = "my_guitar_fx_delay";

#define MAX_DELAY_TIME_MS 800
#define MAX_FEEDBACK 95

#define BUF_SIZE (int)(48000 * MAX_DELAY_TIME_MS / 1000) + 1


float delay(float data, void *args){
    if(args == NULL){
        return data;
    }

    delay_args_t *delay_args = (delay_args_t *)args;

    static bool initialized = false;

    static float coeffs[5];
    static float f = 4000.0f /  48000.0f;
    static float q = 0.707f;
    static float w[2] = {0.0f, 0.0f};

    static float *buf = NULL;
    static int write_index = 0;

    if(!initialized){
        if(buf == NULL){
            buf = heap_caps_calloc(
            BUF_SIZE,
            sizeof(float),
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
            );
        }

        if(buf == NULL){
            ESP_LOGE(TAG, "buffer creation failed");
            return data;
        }

        ESP_ERROR_CHECK(dsps_biquad_gen_lpf_f32(coeffs, f, q));
        
        initialized = true;
    }

    int delay_time = delay_args->delay_time;
    if(delay_time < 0){
        delay_time = 0;
    } else if(delay_time > MAX_DELAY_TIME_MS){
        delay_time = MAX_DELAY_TIME_MS;
    }

    int feedback = delay_args->feedback;

    if(feedback < 0){
        feedback = 0;
    } else if(feedback > MAX_FEEDBACK){
        feedback = MAX_FEEDBACK;
    }

    int mix = delay_args->mix;
    if(mix < 0){
        mix = 0;
    } else if(mix > 100){
        mix = 100;
    }

    int delay_samples = 48000 * delay_time / 1000;

    if(delay_samples < 1){
        delay_samples = 1;
    }

    int read_index = write_index - delay_samples;

    if(read_index < 0){
        read_index += BUF_SIZE;
    }

    float delayed = buf[read_index];

    float feedback_gain = feedback / 100.0f;
    float wet = mix / 100.0f;

    float filtered = 0.0f;
    dsps_biquad_f32(&delayed, &filtered, 1, coeffs, w);

    buf[write_index] = data + filtered * feedback_gain;
    write_index++;

    if(write_index >= BUF_SIZE){
        write_index = 0;
    }

    float out = data * (1.0f - wet) + delayed * wet;
    return out;
}

