#include "ds18b20.h"

#include "esp_timer.h"

#define DS18B20_PIN GPIO_NUM_4
#define CONVERSION_RESOLUTION DS18B20_RESOLUTION_10_BIT

static ds18b20_handle_t handle;

void read_temperature(void *arg) {
    float temp;

    while(true) {
        ds18b20_read_temperature(&handle, &temp);
        ESP_LOGI("temperature", "%f C", temp);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    handle.gpio_pin = DS18B20_PIN;

    ds18b20_config_t config = {
        .trigger_high = DS18B20_MAX_TEMPERATURE,
        .trigger_low = DS18B20_MIN_TEMPERATURE,
        .resolution = CONVERSION_RESOLUTION
    };

    ds18b20_init(&handle, &config);
    ds18b20_get_configuration(&handle, &config);

    ESP_LOGI("rom_code", "family code: %d", handle.rom_code[0]);
    ESP_LOGI("config", "trigger high: %d, trigger low: %d, resolution: %d", config.trigger_high, config.trigger_low, config.resolution);

    xTaskCreate(read_temperature, "temp", 4096, NULL, 10, NULL);
}