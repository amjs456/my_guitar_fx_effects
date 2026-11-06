#include "my_guitar_fx_effects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_check.h"

#include "esp_err.h"
#include "my_guitar_fx_core.h"
#include "portmacro.h"
#include "tuner.h"
#include "delay.h"
#include "dbfs_meter.h"
#include "distortion.h"


void print_dbfs_meter_result_task(void *args){
    QueueHandle_t queue_handle = (QueueHandle_t)args;
    while(1){
        dbfs_result_t dbfs_result;
        xQueueReceive(queue_handle, &dbfs_result, portMAX_DELAY);
        printf("RMS: %.2f dBFS | PEAK: %.2f dBFS\n", dbfs_result.rms_dbfs, dbfs_result.peak_dbfs);
        vTaskDelay(1);
    }
}

void print_tuning_result_task(void *args){
    QueueHandle_t queue_handle = (QueueHandle_t)args;
    while(1){
        TuningResult tuning_result;
        xQueueReceive(queue_handle, &tuning_result, portMAX_DELAY);
        printf("target midi note = %d ,diff_cent = %f\n", tuning_result.target_midi_note, tuning_result.diff_cent);
        vTaskDelay(1);
    }
}

dbfs_meter_args_t dbfs_meter_args;

tuner_arg_t tuner_arg = {
        .reference_frequency_hz = 440.0f,
        .tuning = "Standard",
    };

delay_args_t delay_args = {
    .delay_time = 500,
    .feedback = 95,
    .mix = 50
};

distortion_args_t distortion_args = {
    .gain_db = 20.0f
};


void app_main(void)
{
    QueueHandle_t dbfs_meter_queue_handle = xQueueCreate(1, sizeof(dbfs_result_t));
    dbfs_meter_args.queue_handle = dbfs_meter_queue_handle;

    QueueHandle_t tuning_queue_handle = xQueueCreate(1, sizeof(TuningResult));
    tuner_arg.queue_handle = tuning_queue_handle;

    fx_t fx_array[] = {
        //{dbfs_meter, &dbfs_meter_args},
        //{tuner, &tuner_arg},
        //{delay, &delay_args},
        {distortion, &distortion_args},
    };

    size_t fx_array_length = sizeof(fx_array) / sizeof(fx_array[0]);
    ESP_ERROR_CHECK(my_guitar_fx_core_init(fx_array, fx_array_length));
    ESP_ERROR_CHECK(start_tasks());
    xTaskCreate(print_tuning_result_task, "print_tuning_result_task", 4096, tuning_queue_handle, 5, NULL);
    xTaskCreate(print_dbfs_meter_result_task, "print_dbfs_meter_result_task", 4096, dbfs_meter_queue_handle, 5, NULL);
}
