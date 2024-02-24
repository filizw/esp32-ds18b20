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

static void ds18b20_write_byte(const ds18b20_handler_t *const ds18b20_handler, const uint8_t byte) {
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

void ds18b20_send_command(const ds18b20_handler_t *const ds18b20_handler, const uint8_t command) {
    ds18b20_write_byte(ds18b20_handler, command);
}

void ds18b20_configure(ds18b20_handler_t *const ds18b20_handler, const ds18b20_config_t *const ds18b20_config) {
    if(ds18b20_handler != NULL && ds18b20_config != NULL) {
        ds18b20_reset(ds18b20_handler);
        ds18b20_send_command(ds18b20_handler, DS18B20_SKIP_ROM);
        ds18b20_send_command(ds18b20_handler, DS18B20_WRITE_SCRATCHPAD);

        ds18b20_write_byte(ds18b20_handler, ds18b20_config->trigger_high);
        ds18b20_write_byte(ds18b20_handler, ds18b20_config->trigger_low);
        ds18b20_write_byte(ds18b20_handler, 0 | (ds18b20_config->resolution << 5));

        ds18b20_handler->resolution = ds18b20_handler->resolution;
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or ds18b20_config is NULL");
    }
}

void ds18b20_read_scratchpad(const ds18b20_handler_t *const ds18b20_handler, uint8_t *const buffer, const uint8_t buffer_size) {
    if(ds18b20_handler != NULL && buffer != NULL) {
        ds18b20_reset(ds18b20_handler);
        ds18b20_send_command(ds18b20_handler, DS18B20_SKIP_ROM);
        ds18b20_send_command(ds18b20_handler, DS18B20_READ_SCRATCHPAD);

        for(uint8_t i = 0; i < buffer_size && i < DS18B20_SCRATCHPAD_SIZE; i++) {
            ds18b20_read_byte(ds18b20_handler, &buffer[i]);
        }

        if(buffer_size < DS18B20_SCRATCHPAD_SIZE)
            ds18b20_reset(ds18b20_handler);
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or scratchpad is NULL");
    }
}

void ds18b20_read_temperature(const ds18b20_handler_t *const ds18b20_handler, float *const temperature) {
    if(ds18b20_handler != NULL && temperature != NULL) {
        uint8_t raw_temp[2] = {0};
        uint8_t mask = 0xff;

        ds18b20_reset(ds18b20_handler);
        ds18b20_send_command(ds18b20_handler, DS18B20_SKIP_ROM);
        ds18b20_send_command(ds18b20_handler, DS18B20_CONVERT);

        switch(ds18b20_handler->resolution) {
            case DS18B20_RESOLUTION_9_BIT:
                mask = 0xf8;
                ets_delay_us(93750);
                break;
            case DS18B20_RESOLUTION_10_BIT:
                mask = 0xfc;
                ets_delay_us(187500);
                break;
            case DS18B20_RESOLUTION_11_BIT:
                mask = 0xfe;
                ets_delay_us(375000);
                break;
            case DS18B20_RESOLUTION_12_BIT:
                ets_delay_us(750000);
                break;
        }

        ds18b20_read_scratchpad(ds18b20_handler, raw_temp, 2);

        *temperature = ((raw_temp[1] << 8) | (raw_temp[0] & mask)) / 16.0f;
    }
    else {
        ESP_LOGE(TAG, "ds18b20_handler or temperature is NULL");
    }
}