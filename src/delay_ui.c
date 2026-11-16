#include "delay_ui.h"

#include "lvgl.h"
#include "delay.h"

static void delay_slider_cb(lv_event_t *e){
    lv_obj_t *slider = lv_event_get_target(e);
    delay_args_t *delay_args = (delay_args_t *)lv_event_get_user_data(e);
    int32_t delay_time = lv_slider_get_value(slider);
    delay_args->delay_time = delay_time;
}


lv_obj_t *delay_ui_create(lv_obj_t *parent, void *args){
    delay_args_t *delay_args = (delay_args_t *)args;

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_name(container, "Delay");
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *delay_time_slider = lv_slider_create(container);
    lv_slider_set_min_value(delay_time_slider, 0);
    lv_slider_set_max_value(delay_time_slider, 800);
    lv_obj_add_event_cb(delay_time_slider, delay_slider_cb, LV_EVENT_VALUE_CHANGED, args);

    return container;
}