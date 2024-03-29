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
    ds18b20_config_t config = {
        .gpio_pin = DS18B20_PIN,
        .enable_crc = DS18B20_CRC_ENABLE,
        .trigger_high = DS18B20_MAX_TEMPERATURE,
        .trigger_low = DS18B20_MIN_TEMPERATURE,
        .resolution = CONVERSION_RESOLUTION
    };

    if(!ds18b20_error_check(ds18b20_init(&handle, &config), "main", NULL)) {
        ds18b20_get_configuration(&handle, &config);

        ESP_LOGI("rom_code", "family code: %d", handle.rom_code[0]);
        ESP_LOGI("config", "trigger high: %d, trigger low: %d, resolution: %d", config.trigger_high, config.trigger_low, config.resolution);
        ESP_LOGI("scratchpad CRC", "%d", handle.scratchpad[DS18B20_SCRATCHPAD_SIZE - 1]);

        xTaskCreate(read_temperature, "temp", 4096, NULL, 10, NULL);
    }
}