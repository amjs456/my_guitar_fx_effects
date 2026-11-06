#include "distortion.h"

#include <math.h>

float distortion(float data, void *args){
    distortion_args_t *distortion_args = (distortion_args_t *)args;
    float gain_db = distortion_args->gain_db;
    float gain = powf(10.0f, gain_db / 20.0f);
    float gained = data * gain;
    float distorted = tanhf(gained);
    return distorted;
}