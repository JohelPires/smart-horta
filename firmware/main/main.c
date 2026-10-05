#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "dht.h"

#define RELAY_GPIO           GPIO_NUM_18
#define STATUS_LED_GPIO      GPIO_NUM_18
#define IRRIGAR_BTN_GPIO     GPIO_NUM_23
#define DHT_GPIO             GPIO_NUM_15
#define DHT_SENSOR_TYPE      DHT_TYPE_AM2301
#define DHT_INTERVAL_MS      2000
#define IRRIGATION_PULSE_MS  5000

static const char *TAG = "smart_horta";

static bool s_irrigating;

static void irrigar_btn_task(void *arg)
{
    bool prev = true;
    for (;;) {
        bool now = gpio_get_level(IRRIGAR_BTN_GPIO) == 0;
        if (now && !prev && !s_irrigating) {
            s_irrigating = true;
            gpio_set_level(RELAY_GPIO, 1);
            gpio_set_level(STATUS_LED_GPIO, 1);
            ESP_LOGI(TAG, "IRRIGAR ON (manual, %d ms)", IRRIGATION_PULSE_MS);

            vTaskDelay(pdMS_TO_TICKS(IRRIGATION_PULSE_MS));

            gpio_set_level(RELAY_GPIO, 0);
            gpio_set_level(STATUS_LED_GPIO, 0);
            ESP_LOGI(TAG, "IRRIGAR OFF (auto)");
            s_irrigating = false;
        }
        prev = now;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void dht_task(void *arg)
{
    for (;;) {
        float humidity, temperature;
        esp_err_t err = dht_read_float_data(DHT_SENSOR_TYPE, DHT_GPIO,
                                            &humidity, &temperature);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "temp=%.1fC hum=%.1f%%", temperature, humidity);
        } else {
            ESP_LOGW(TAG, "DHT read failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(DHT_INTERVAL_MS));
    }
}

void app_main(void)
{
    s_irrigating = false;

    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO, 0);

    gpio_reset_pin(STATUS_LED_GPIO);
    gpio_set_direction(STATUS_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(STATUS_LED_GPIO, 0);

    gpio_reset_pin(IRRIGAR_BTN_GPIO);
    gpio_set_direction(IRRIGAR_BTN_GPIO, GPIO_MODE_INPUT);
    gpio_pullup_en(IRRIGAR_BTN_GPIO);

    ESP_LOGI(TAG, "boot ok, IRRIGAR OFF (button = %d ms pulse)", IRRIGATION_PULSE_MS);

    xTaskCreate(irrigar_btn_task, "irrigar_btn", 2048, NULL, 5, NULL);
    xTaskCreate(dht_task, "dht", 2048, NULL, 5, NULL);
}
