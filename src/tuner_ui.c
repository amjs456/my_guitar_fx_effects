#include "tuner_ui.h"

#include "lvgl.h"
#include "tuner.h"

QueueHandle_t tuner_result_queue_handle;
lv_obj_t *target_note_lbl;
lv_obj_t *diff_cent_lbl;


static char notes[12][3] = {
    "C", "C#", "D", "D#", "E", "F", "F#","G", "G#", "A", "A#", "B"
};

static void tuner_update_cb(lv_timer_t *timer){
    (void)timer;
    
    static TickType_t last_recieve_tick = 0;
    static bool is_displaying = false;

    TuningResult tuning_result;

    if(xQueueReceive(tuner_result_queue_handle, &tuning_result, 0) == pdTRUE){
        last_recieve_tick = xTaskGetTickCount();
        is_displaying = true;
        int mod = tuning_result.target_midi_note % 12;
        lv_label_set_text(target_note_lbl, notes[mod]);

        char diff_cent[32];
        sprintf(diff_cent, "%.2f", tuning_result.diff_cent); 
        lv_label_set_text(diff_cent_lbl, diff_cent);
    } else {
        if(is_displaying && xTaskGetTickCount() - last_recieve_tick >= pdMS_TO_TICKS(500)){
            lv_label_set_text(target_note_lbl, "---");
            lv_label_set_text(diff_cent_lbl, "---");
        }
    }
}

lv_obj_t *tuner_ui_create(lv_obj_t *parent, void *args){

    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_name(container, "Tuner");

    lv_obj_t *title_lbl = lv_label_create(container);
    lv_label_set_text(title_lbl, "Tuner");

    tuner_arg_t *tuner_args = (tuner_arg_t *)args;
    tuner_result_queue_handle = tuner_args->queue_handle;
    //lv_obj_t *tuner_mode_lbl = lv_label_create(tuner_container);
    //lv_label_set_text(tuner_mode_lbl, args->tuning);
    
    lv_obj_t *tuner_result_container = lv_obj_create(container);
    lv_obj_set_style_pad_all(tuner_result_container, 0, 0);
    lv_obj_set_flex_flow(tuner_result_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(tuner_result_container);

    target_note_lbl = lv_label_create(tuner_result_container);
    lv_obj_set_style_text_font(target_note_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(target_note_lbl);
    lv_label_set_text(target_note_lbl, "---");

    diff_cent_lbl = lv_label_create(tuner_result_container);
    lv_obj_center(diff_cent_lbl);
    lv_label_set_text(diff_cent_lbl, "---");

    lv_timer_create(tuner_update_cb, 50, NULL);

    return container;
}