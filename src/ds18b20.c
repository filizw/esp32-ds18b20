#include "ds18b20.h"

static const char *TAG = "ds18b20";

static const uint32_t LEVEL_LOW = 0;
//static const uint32_t LEVEL_HIGH = 1;

static const uint32_t OW_RESET_PULSE_TIME = 480;
static const uint32_t OW_PRESENCE_PULSE_DELAY_TIME = 70;
static const uint32_t OW_PRESENCE_PULSE_AND_RECOVERY_TIME = 410;

void ds18b20_init(const ds18b20_handler_t *const ds18b20_handler) {
    if(ds18b20_handler != NULL) {
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(ds18b20_handler->ow_pin, LEVEL_LOW);
        ets_delay_us(OW_RESET_PULSE_TIME);
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_INPUT);
        ets_delay_us(OW_PRESENCE_PULSE_DELAY_TIME);
        const int presence_level = gpio_get_level(ds18b20_handler->ow_pin);
        ets_delay_us(OW_PRESENCE_PULSE_AND_RECOVERY_TIME);
        const int recovery_level = gpio_get_level(ds18b20_handler->ow_pin);

        ESP_LOGD(TAG, "reset: presence level: %d, recovery level: %d", presence_level, recovery_level);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler is NULL");
    }
}