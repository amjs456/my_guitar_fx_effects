#include "freertos/idf_additions.h"
#include "my_guitar_fx_effects.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_check.h"

#include "esp_err.h"
#include "my_guitar_fx_core.h"
#include "portmacro.h"
#include "tuner.h"

void print_tuning_result_task(void *args){
    QueueHandle_t queue_handle = (QueueHandle_t)args;
    while(1){
        TuningResult tuning_result;
        xQueueReceive(queue_handle, &tuning_result, portMAX_DELAY);
        printf("target midi note = %d ,diff_cent = %f\n", tuning_result.target_midi_note, tuning_result.diff_cent);
        vTaskDelay(1);
    }
}

tuner_arg_t tuner_arg = {
        .reference_frequency_hz = 440.0f,
        .tuning = "Standard",
    };

void app_main(void)
{
    QueueHandle_t queue_handle = xQueueCreate(1, sizeof(TuningResult));
    tuner_arg.queue_handle = queue_handle;

    fx_t fx_array[] = {
        {tuner, &tuner_arg},
    };

    size_t fx_array_length = sizeof(fx_array) / sizeof(fx_array[0]);
    ESP_ERROR_CHECK(my_guitar_fx_core_init(fx_array, fx_array_length));
    ESP_ERROR_CHECK(start_tasks());
    xTaskCreate(print_tuning_result_task, "print_tuning_result_task", 4096, queue_handle, 5, NULL);
}
