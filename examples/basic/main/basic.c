#include "my_guitar_fx_effects.h"

#include "esp_check.h"

#include "esp_err.h"
#include "my_guitar_fx_core.h"

void app_main(void)
{
    FxFuncPtr const fx_func_array[] = {
        tuner,
    };

    size_t fx_func_array_length = sizeof(fx_func_array) / sizeof(fx_func_array[0]);
    ESP_ERROR_CHECK(my_guitar_fx_core_init(fx_func_array, fx_func_array_length));
    ESP_ERROR_CHECK(start_tasks());
}
