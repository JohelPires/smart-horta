#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BLINK_GPIO GPIO_NUM_4

static const char *TAG = "smart_horta";

void app_main(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    int level = 0;
    for (;;) {
        level = !level;
        gpio_set_level(BLINK_GPIO, level);
        ESP_LOGI(TAG, "LED %s", level ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
