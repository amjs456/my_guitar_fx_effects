#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

typedef struct {
    int delay_time;
    int feedback;
    int mix;
} delay_args_t;

#endif