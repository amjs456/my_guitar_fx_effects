#include "my_guitar_fx_effects.h"
#include <stdint.h>
#include <stdio.h>


#define SAMPLE_SIZE 8192
#define HOP_SIZE 1024

uint32_t tuner(uint32_t data){
    static int head = 0;
    static uint32_t samples[SAMPLE_SIZE];

    if(head<=SAMPLE_SIZE){
        samples[head] = data;
        head++;
    } else {
        printf("%ld\n", samples[0]);
        head = 0;
    }

    return data;
}