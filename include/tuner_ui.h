#ifndef TUNER_UI_H
#define TUNER_UI_H

#include "lvgl.h"

void button_event_cb(lv_event_t *e);
lv_obj_t *tuner_ui_create(lv_obj_t *parent, void *args);


#endif