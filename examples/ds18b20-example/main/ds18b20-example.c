#include "ds18b20.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DS18B20_PIN GPIO_NUM_4
#define CONVERSION_RESOLUTION DS18B20_RESOLUTION_10_BIT

static ds18b20_handle_t handle = NULL;

void read_temperature(void *arg) {
    float temp;

    while(true) {
        ds18b20_read_temperature(handle, &temp);
        ESP_LOGI("temperature", "%f C", temp);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    handle = ds18b20_create_handle(DS18B20_PIN);

    ds18b20_config_t config = {
        .enable_crc = DS18B20_CRC_ENABLE,
        .trigger_high = DS18B20_MAX_TEMPERATURE,
        .trigger_low = DS18B20_MIN_TEMPERATURE,
        .resolution = CONVERSION_RESOLUTION
    };

    if(!ds18b20_init(handle, &config)) {
        xTaskCreate(read_temperature, "temp", 4096, NULL, 10, NULL);
    }
    else {
        ESP_LOGE("main", "ds18b20 init failed");
    }
}
