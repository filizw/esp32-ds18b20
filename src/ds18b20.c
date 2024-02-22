#include "ds18b20.h"

static const char *TAG = "ds18b20";

static const uint32_t LEVEL_LOW = 0;
//static const uint32_t LEVEL_HIGH = 1;

static void ds18b20_write_0(const ds18b20_handler_t *const ds18b20_handler) {
    if(ds18b20_handler != NULL) {
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(ds18b20_handler->ow_pin, LEVEL_LOW);
        ets_delay_us(60);
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_INPUT);
        ets_delay_us(2);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler is NULL");
    }
}

static void ds18b20_write_1(const ds18b20_handler_t *const ds18b20_handler) {
    if(ds18b20_handler != NULL) {
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(ds18b20_handler->ow_pin, LEVEL_LOW);
        ets_delay_us(2);
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_INPUT);
        ets_delay_us(60);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler is NULL");
    }
}

static void ds18b20_read_bit(const ds18b20_handler_t *const ds18b20_handler, uint8_t *const bit) {
    if(ds18b20_handler != NULL && bit != NULL) {
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(ds18b20_handler->ow_pin, LEVEL_LOW);
        ets_delay_us(2);
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_INPUT);
        ets_delay_us(8);
        *bit = gpio_get_level(ds18b20_handler->ow_pin);
        ets_delay_us(52);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or bit is NULL");
    }
}

static void ds18b20_write_byte(const ds18b20_handler_t *const ds18b20_handler, uint8_t byte) {
    if(ds18b20_handler != NULL) {
        for(uint8_t i = 0; i < 8; i++) {
            if(((byte >> i) & 1) == 0)
                ds18b20_write_0(ds18b20_handler);
            else
                ds18b20_write_1(ds18b20_handler);
        }
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or bit is NULL");
    }
}

static void ds18b20_read_byte(const ds18b20_handler_t *const ds18b20_handler, uint8_t *const byte) {
    if(ds18b20_handler != NULL) {
        *byte = 0;
        uint8_t bit = 0;

        for(uint8_t i = 0; i < 8; i++) {
            ds18b20_read_bit(ds18b20_handler, &bit);
            *byte |= (bit << i);
        }
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or bit is NULL");
    }
}

void ds18b20_reset(const ds18b20_handler_t *const ds18b20_handler) {
    if(ds18b20_handler != NULL) {
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(ds18b20_handler->ow_pin, LEVEL_LOW);
        ets_delay_us(480);
        gpio_set_direction(ds18b20_handler->ow_pin, GPIO_MODE_INPUT);
        ets_delay_us(70);
        const int presence_level = gpio_get_level(ds18b20_handler->ow_pin);
        ets_delay_us(410);
        const int recovery_level = gpio_get_level(ds18b20_handler->ow_pin);

        ESP_LOGD(TAG, "reset: presence level: %d, recovery level: %d", presence_level, recovery_level);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler is NULL");
    }
}

void ds18b20_send_command(const ds18b20_handler_t *const ds18b20_handler, uint8_t command) {
    ds18b20_write_byte(ds18b20_handler, command);
}

void ds18b20_get_raw_temperature(const ds18b20_handler_t *const ds18b20_handler, uint16_t *const raw_temp) {
    uint8_t ls_byte, ms_byte;
    ls_byte = ms_byte = 0;
    ds18b20_read_byte(ds18b20_handler, &ls_byte);
    ds18b20_read_byte(ds18b20_handler, &ms_byte);

    ESP_LOGD(TAG, "ls_byte: %d, ms_byte: %d", ls_byte, ms_byte);

    *raw_temp = 0;
    *raw_temp |= (ms_byte << 8) | ls_byte;
}