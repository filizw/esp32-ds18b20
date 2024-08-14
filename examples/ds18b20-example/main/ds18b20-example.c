#include "ds18b20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DS18B20_PIN GPIO_NUM_4

void read_temperature(void *arg) {
    ds18b20_handle_t handle;

    ds18b20_config_t config = {
        .owb_pin = DS18B20_PIN,
        .enable_crc = DS18B20_CRC_ENABLE,
        .trigger_high = DS18B20_TRIGGER_HIGH_DEFAULT,
        .trigger_low = DS18B20_TRIGGER_LOW_DEFAULT,
        .resolution = DS18B20_RESOLUTION_10_BIT
    };

    ESP_ERROR_CHECK(ds18b20_init(&handle, &config));
    //ESP_ERROR_CHECK(ds18b20_init_default(&handle, DS18B20_PIN));

    float temperature;

    while(true) {
        ds18b20_read_temperature(handle, &temperature);
        ESP_LOGI("DS18B20", "temperature = %f C", temperature);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    xTaskCreate(read_temperature, "read_temperature", 4096, NULL, 10, NULL);
}
