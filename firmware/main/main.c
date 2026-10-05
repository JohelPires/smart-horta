#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BLINK_GPIO       GPIO_NUM_4
#define IRRIGAR_LED_GPIO GPIO_NUM_18
#define IRRIGAR_BTN_GPIO GPIO_NUM_23

static const char *TAG = "smart_horta";

static bool s_irrigar_on;

static void blink_task(void *arg)
{
    int level = 0;
    for (;;) {
        level = !level;
        gpio_set_level(BLINK_GPIO, level);
        ESP_LOGI(TAG, "LED %s", level ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void irrigar_btn_task(void *arg)
{
    bool prev = true;
    for (;;) {
        bool now = gpio_get_level(IRRIGAR_BTN_GPIO) == 0;
        if (now && !prev) {
            s_irrigar_on = !s_irrigar_on;
            gpio_set_level(IRRIGAR_LED_GPIO, s_irrigar_on);
            ESP_LOGI(TAG, "IRRIGAR %s (manual)", s_irrigar_on ? "ON" : "OFF");
        }
        prev = now;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    s_irrigar_on = false;

    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);

    gpio_reset_pin(IRRIGAR_LED_GPIO);
    gpio_set_direction(IRRIGAR_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(IRRIGAR_LED_GPIO, 0);

    gpio_reset_pin(IRRIGAR_BTN_GPIO);
    gpio_set_direction(IRRIGAR_BTN_GPIO, GPIO_MODE_INPUT);
    gpio_pullup_en(IRRIGAR_BTN_GPIO);

    ESP_LOGI(TAG, "boot ok, IRRIGAR OFF (press button to toggle)");

    xTaskCreate(blink_task, "blink", 2048, NULL, 5, NULL);
    xTaskCreate(irrigar_btn_task, "irrigar_btn", 2048, NULL, 5, NULL);
}
