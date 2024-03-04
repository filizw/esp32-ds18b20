#include "ds18b20.h"

static const char *TAG = "ds18b20";

static const uint32_t LEVEL_LOW = 0;
//static const uint32_t LEVEL_HIGH = 1;

//static const uint32_t conversion_time_us[] = {93750, 187500, 375000, 750000};

static ds18b20_error_t ds18b20_write_0(const ds18b20_handle_t *const handle) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->gpio_pin, LEVEL_LOW);
    ets_delay_us(60);
    gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT);
    ets_delay_us(2);

    return DS18B20_OK;
}

static ds18b20_error_t ds18b20_write_1(const ds18b20_handle_t *const handle) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->gpio_pin, LEVEL_LOW);
    ets_delay_us(2);
    gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT);
    ets_delay_us(60);

    return DS18B20_OK;
}

static ds18b20_error_t ds18b20_read_bit(const ds18b20_handle_t *const handle, uint8_t *const bit) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(bit == NULL)
        return DS18B20_ERROR_INVALID_ARGUMENT;

    gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->gpio_pin, LEVEL_LOW);
    ets_delay_us(2);
    gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT);
    ets_delay_us(8);
    *bit = gpio_get_level(handle->gpio_pin);
    ets_delay_us(52);

    return DS18B20_OK;
}

static ds18b20_error_t ds18b20_write_byte(const ds18b20_handle_t *const handle, const uint8_t byte) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    for(uint8_t i = 0; i < 8; i++) {
        if(((byte >> i) & 1) == 0)
            ds18b20_write_0(handle);
        else
            ds18b20_write_1(handle);
    }

    return DS18B20_OK;
}

static ds18b20_error_t ds18b20_read_byte(const ds18b20_handle_t *const handle, uint8_t *const byte) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(byte == NULL)
        return DS18B20_ERROR_INVALID_ARGUMENT;

    *byte = 0;
    uint8_t bit = 0;

    for(uint8_t i = 0; i < 8; i++) {
        ds18b20_read_bit(handle, &bit);
        *byte |= (bit << i);
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_reset(const ds18b20_handle_t *const handle) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->gpio_pin, LEVEL_LOW);
    ets_delay_us(480);
    gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT);
    ets_delay_us(70);
    const int presence_level = gpio_get_level(handle->gpio_pin);
    ets_delay_us(410);
    const int recovery_level = gpio_get_level(handle->gpio_pin);

    ESP_LOGD(TAG, "reset: presence level: %d, recovery level: %d", presence_level, recovery_level);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_send_command(const ds18b20_handle_t *const handle, const uint8_t command) {
    ds18b20_write_byte(handle, command);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_rom(ds18b20_handle_t *const handle) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    ds18b20_reset(handle);
    ds18b20_send_command(handle, DS18B20_COMMAND_READ_ROM);

    for(uint8_t i = 0; i < DS18B20_ROM_CODE_SIZE; i++) {
        ds18b20_read_byte(handle, &handle->rom_code[i]);
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_write_scratchpad(const ds18b20_handle_t *const handle, const uint8_t *const data) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(data == NULL)
        return DS18B20_ERROR_INVALID_ARGUMENT;

    ds18b20_reset(handle);
    ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM);
    ds18b20_send_command(handle, DS18B20_COMMAND_WRITE_SCRATCHPAD);

    ds18b20_write_byte(handle, data[0]);
    ds18b20_write_byte(handle, data[1]);
    ds18b20_write_byte(handle, data[2]);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_scratchpad(ds18b20_handle_t *const handle, const uint8_t num_of_bytes_to_read) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    ds18b20_reset(handle);
    ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM);
    ds18b20_send_command(handle, DS18B20_COMMAND_READ_SCRATCHPAD);

    for(uint8_t i = 0; i < num_of_bytes_to_read && i < DS18B20_SCRATCHPAD_SIZE; i++) {
        ds18b20_read_byte(handle, &handle->scratchpad[i]);
    }

    if(num_of_bytes_to_read < DS18B20_SCRATCHPAD_SIZE)
        ds18b20_reset(handle);
    
    return DS18B20_OK;
}

ds18b20_error_t ds18b20_init(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(config == NULL)
        return DS18B20_ERROR_INVALID_CONFIG;

    ds18b20_configure(handle, config);

    ds18b20_read_rom(handle);
    ds18b20_read_scratchpad(handle, DS18B20_SCRATCHPAD_SIZE);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_configure(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(config == NULL)
        return DS18B20_ERROR_INVALID_CONFIG;

    const uint8_t data[] = {
        config->trigger_high,
        config->trigger_low,
        config->resolution
    };

    ds18b20_write_scratchpad(handle, data);

    ds18b20_read_scratchpad(handle, DS18B20_SCRATCHPAD_SIZE);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_temperature(ds18b20_handle_t *const handle, float *const temperature) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;

    ds18b20_reset(handle);
    ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM);
    ds18b20_send_command(handle, DS18B20_COMMAND_CONVERT);

    ets_delay_us(DS18B20_MAX_CONVERSION_TIME >> (3 - (handle->scratchpad[4] >> 5)));

    ds18b20_read_scratchpad(handle, 2);

    if(temperature != NULL) {
        const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5));
        *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f;
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_get_temperature(const ds18b20_handle_t *const handle, float *const temperature) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(temperature == NULL)
        return DS18B20_ERROR_INVALID_ARGUMENT;

    const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5));
    *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f;

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_get_configuration(const ds18b20_handle_t *const handle, ds18b20_config_t *const config) {
    if(handle == NULL)
        return DS18B20_ERROR_INVALID_HANDLE;
    
    if(config == NULL)
        return DS18B20_ERROR_INVALID_CONFIG;

    config->trigger_high = handle->scratchpad[2];
    config->trigger_low = handle->scratchpad[3];
    config->resolution = handle->scratchpad[4];

    return DS18B20_OK;
}