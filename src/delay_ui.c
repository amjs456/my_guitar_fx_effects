#include "delay_ui.h"

#include "lvgl.h"
#include "delay.h"
#include "misc/lv_area.h"
#include "my_guitar_fx_ui_types.h"
#include "slider.h"


lv_obj_t *delay_ui_create(lv_obj_t *parent, void *args){
    delay_args_t *delay_args = (delay_args_t *)args;

    slider_config_t configs[] = {
        {"TIME", "ms",0, 800, &delay_args->delay_time},
        {"FEEDBACK", "%", 0, 95, &delay_args->feedback},
        {"MIX", "%", 0, 100, &delay_args->mix}
    };

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_name(container, "Delay");
    lv_obj_set_size(container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);


    const size_t config_count = sizeof(configs)/ sizeof(slider_config_t);
    lv_obj_t *slider_contaier = create_slider(container, configs, config_count);
    return container;
}