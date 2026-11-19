#include "delay_ui.h"

#include <stdio.h>

#include "lvgl.h"
#include "delay.h"

typedef struct {
    const char *title;
    const char *unit;
    int min;
    int max;
    int *value; 
} slider_config_t;

typedef struct {
    int *value;
    lv_obj_t *value_label;
} slider_event_cb_user_data_t;

    slider_event_cb_user_data_t slider_event_cb_user_data[CONFIG_MAX_FX_PARAM_COUNT];

static void slider_event_cb(lv_event_t *e){
    slider_event_cb_user_data_t *slider_event_cb_user_data = (slider_event_cb_user_data_t *)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    *slider_event_cb_user_data->value = (int)lv_slider_get_value(slider);

    char value[4];
    sprintf(value, "%d",(int)lv_slider_get_value(slider));
    lv_label_set_text(slider_event_cb_user_data->value_label, value);
}


lv_obj_t *delay_ui_create(lv_obj_t *parent, void *args){
    delay_args_t *delay_args = (delay_args_t *)args;

    slider_config_t configs[] = {
        {"TIME", "ms",0, 800, &delay_args->delay_time},
        {"FEEDBACK", "%", 0, 95, &delay_args->feedback},
        {"MIX", "%", 0, 100, &delay_args->mix}
    };

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_name(container, "Delay");
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);

    const size_t config_count = sizeof(configs)/ sizeof(slider_config_t);
    for(int i = 0; i < config_count; i++){
        lv_obj_t *row = lv_obj_create(container);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_set_size(row, lv_pct(100), 65);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *title_label = lv_label_create(row);
        lv_label_set_text(title_label, configs[i].title);
        lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *value_container = lv_obj_create(row);
        lv_obj_set_style_pad_all(value_container, 0, 0);
        lv_obj_align(value_container, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_size(value_container, lv_pct(10), lv_pct(50));

        lv_obj_t *value_label = lv_label_create(value_container);
        lv_obj_align(value_label, LV_ALIGN_LEFT_MID, 0, 0);
        char value_str[4];
        sprintf(value_str, "%d", *configs[i].value);
        lv_label_set_text(value_label, value_str);
        lv_obj_set_width(value_container, lv_pct(20));
        
        lv_obj_t *unit_label = lv_label_create(value_container);
        lv_label_set_text(unit_label, configs[i].unit);
        lv_obj_align(unit_label, LV_ALIGN_RIGHT_MID, 0, 0);


        lv_obj_t *slider = lv_slider_create(row);
        lv_obj_set_width(slider, lv_pct(85));
        lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -2);

        lv_slider_set_range(slider, configs[i].min, configs[i].max);
        lv_slider_set_value(slider, *configs[i].value, LV_ANIM_OFF);

        slider_event_cb_user_data[i].value = configs[i].value;
        slider_event_cb_user_data[i].value_label = value_label;
        lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, &slider_event_cb_user_data[i]);

    };

    return container;
}