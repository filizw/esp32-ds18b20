#include "ds18b20.h"

static const char *const ds18b20_error_strings[] = {
    "DS18B20_OK",
    "DS18B20_ERROR_HANDLE_NULL",
    "DS18B20_ERROR_CONFIG_NULL",
    "DS18B20_ERROR_ARGUMENT_NULL",
    "DS18B20_ERROR_RESET_FAILED",
    "DS18B20_ERROR_READ_ROM_FAILED",
    "DS18B20_ERROR_WRITE_SCRATCHPAD_FAILED",
    "DS18B20_ERROR_READ_SCRATCHPAD_FAILED",
    "DS18B20_ERROR_INIT_FAILED",
    "DS18B20_ERROR_READ_TEMPERATURE_FAILED",
    "DS18B20_ERROR_CONFIGURATION_FAILED",
    "DS18B20_ERROR_NOT_INITIALIZED"
};

static void ds18b20_write_0(const ds18b20_handle_t *const handle) {
    if(handle) {
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(handle->gpio_pin, 0); // pulling bus low for 60us
        ets_delay_us(60);
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT); // releasing bus
        ets_delay_us(2);
    }
}

static void ds18b20_write_1(const ds18b20_handle_t *const handle) {
    if(handle) {
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(handle->gpio_pin, 0); // pulling bus low for 2us
        ets_delay_us(2);
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT); // releasing bus
        ets_delay_us(60);
    }
}

static void ds18b20_read_bit(const ds18b20_handle_t *const handle, uint8_t *const bit) {
    if(handle && bit) {
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(handle->gpio_pin, 0); // pulling bus low fow 2us
        ets_delay_us(2);
        gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT); // releasing bus
        ets_delay_us(8);
        *bit = gpio_get_level(handle->gpio_pin); // storing value send by the DS18B20
        ets_delay_us(52);
    }
}

static void ds18b20_write_byte(const ds18b20_handle_t *const handle, const uint8_t byte) {
    if(handle) {
        for(uint8_t i = 0; i < 8; i++) { // iterating through single bits and sending them to the DS18B20
            if(((byte >> i) & 1) == 0)
                ds18b20_write_0(handle);
            else
                ds18b20_write_1(handle);
        }
    }
}

static void ds18b20_read_byte(const ds18b20_handle_t *const handle, uint8_t *const byte) {
    if(handle && byte) {
        *byte = 0;
        uint8_t bit = 0;

        for(uint8_t i = 0; i < 8; i++) { // reading one bit at a time and storing it in one byte
            ds18b20_read_bit(handle, &bit);
            *byte |= (bit << i);
        }
    }
}

ds18b20_error_t ds18b20_reset(const ds18b20_handle_t *const handle) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;

    gpio_set_direction(handle->gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->gpio_pin, 0); // reset pulse, pulling bus low for 480us
    ets_delay_us(480);
    gpio_set_direction(handle->gpio_pin, GPIO_MODE_INPUT); // releasing bus
    ets_delay_us(70); // waiting for presense pulse
    const int presence_level = gpio_get_level(handle->gpio_pin);
    ets_delay_us(410);
    const int recovery_level = gpio_get_level(handle->gpio_pin);

    if(!presence_level && recovery_level) {
        return DS18B20_OK;
    }
    else {
        return DS18B20_ERROR_RESET_FAILED;
    }
}

ds18b20_error_t ds18b20_send_command(const ds18b20_handle_t *const handle, const uint8_t command) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;

    ds18b20_write_byte(handle, command);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_rom(ds18b20_handle_t *const handle) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;

    if(ds18b20_error_check(ds18b20_reset(handle), DS18B20_TAG, "read_rom/reset: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_READ_ROM), DS18B20_TAG, "read_rom/send_command: "))
        return DS18B20_ERROR_READ_ROM_FAILED;

    for(uint8_t i = 0; i < DS18B20_ROM_CODE_SIZE; i++) { // reading one byte of the ROM code at a time
        ds18b20_read_byte(handle, &handle->rom_code[i]);
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_write_scratchpad(const ds18b20_handle_t *const handle, const uint8_t *const data) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;
    
    if(!data)
        return DS18B20_ERROR_ARGUMENT_NULL;

    if(ds18b20_error_check(ds18b20_reset(handle), DS18B20_TAG, "write_scratchpad/reset: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM), DS18B20_TAG, "write_scratchpad/send_command: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_WRITE_SCRATCHPAD), DS18B20_TAG, "write_scratchpad/send_command: "))
        return DS18B20_ERROR_WRITE_SCRATCHPAD_FAILED;

    ds18b20_write_byte(handle, data[0]); // writing to th register
    ds18b20_write_byte(handle, data[1]); // writing to tl register
    ds18b20_write_byte(handle, data[2]); // writing to configuration register

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_scratchpad(ds18b20_handle_t *const handle, const uint8_t num_of_bytes_to_read) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;

    if(ds18b20_error_check(ds18b20_reset(handle), DS18B20_TAG, "read_scratchpad/reset: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM), DS18B20_TAG, "read_scratchpad/send_command: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_READ_SCRATCHPAD), DS18B20_TAG, "read_scratchpad/send_command: "))
        return DS18B20_ERROR_READ_SCRATCHPAD_FAILED;

    for(uint8_t i = 0; i < num_of_bytes_to_read && i < DS18B20_SCRATCHPAD_SIZE; i++) { // reading one byte of the scratchpad at a time
        ds18b20_read_byte(handle, &handle->scratchpad[i]);
    }

    if(num_of_bytes_to_read < DS18B20_SCRATCHPAD_SIZE) // if not reading the whole scratchpad then issue a reset
        ds18b20_reset(handle);
    
    return DS18B20_OK;
}

ds18b20_error_t ds18b20_init(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    handle->is_init = true;

    if(!handle) {
        handle->is_init = false;
        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!config) {
        handle->is_init = false;
        return DS18B20_ERROR_CONFIG_NULL;
    }
    
    handle->gpio_pin = config->gpio_pin;

    if(ds18b20_error_check(ds18b20_configure(handle, config), DS18B20_TAG, "init/configure: ") || // send configuation to the DS18B20
       ds18b20_error_check(ds18b20_read_rom(handle), DS18B20_TAG, "init/read_rom: ")) // read ROM code and store it in a handle
        return DS18B20_ERROR_INIT_FAILED;
    
    return DS18B20_OK;
}

ds18b20_error_t ds18b20_init_default(ds18b20_handle_t *const handle, const gpio_num_t gpio_pin) {
    const ds18b20_config_t config = { // default settings
        .gpio_pin = gpio_pin,
        .trigger_high = DS18B20_TRIGGER_HIGH_DEFAULT,
        .trigger_low = DS18B20_TRIGGER_LOW_DEFAULT,
        .resolution = DS18B20_RESOLUTION_DEFAULT
    };

    return ds18b20_init(handle, &config);
}

ds18b20_error_t ds18b20_configure(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;
    
    if(!config)
        return DS18B20_ERROR_CONFIG_NULL;

    const uint8_t data[] = { // casting to the appropriate data type
        config->trigger_high,
        config->trigger_low,
        config->resolution
    };

    if(ds18b20_error_check(ds18b20_write_scratchpad(handle, data), DS18B20_TAG, "configure/write_scratchpad: ") || // writing configuration to the DS18B20
       ds18b20_error_check(ds18b20_read_scratchpad(handle, DS18B20_SCRATCHPAD_SIZE), DS18B20_TAG, "configure/read_scratchpad: ")) // reading written configuration
        return DS18B20_ERROR_CONFIGURATION_FAILED;

    if((int8_t)handle->scratchpad[2] != config->trigger_high ||
       (int8_t)handle->scratchpad[3] != config->trigger_low ||
       (ds18b20_resolution_t)handle->scratchpad[4] != config->resolution) {
        return DS18B20_ERROR_CONFIGURATION_FAILED;
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_temperature(ds18b20_handle_t *const handle, float *const temperature) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;

    if(ds18b20_error_check(ds18b20_reset(handle), DS18B20_TAG, "read_temperature/reset: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM), DS18B20_TAG, "read_temperature/send_command: ") ||
       ds18b20_error_check(ds18b20_send_command(handle, DS18B20_COMMAND_CONVERT), DS18B20_TAG, "read_temperature/send_command: "))
        return DS18B20_ERROR_READ_TEMPERATURE_FAILED;

    ets_delay_us(DS18B20_MAX_CONVERSION_TIME >> (3 - (handle->scratchpad[4] >> 5))); // calculating conversion time using a resolution value

    if(ds18b20_error_check(ds18b20_read_scratchpad(handle, 2), DS18B20_TAG, "read_temperature/read_scratchpad: ")) // reading only a temperature value
        return DS18B20_ERROR_READ_TEMPERATURE_FAILED;
    
    if(temperature != NULL) {
        const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5)); // calculating mask that depends on a resolution
        *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f; // converting raw temperature to degrees
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_get_temperature(const ds18b20_handle_t *const handle, float *const temperature) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;

    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;
    
    if(!temperature)
        return DS18B20_ERROR_ARGUMENT_NULL;

    const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5)); // calculating mask that depends on a resolution
    *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f; // converting raw temperature to degrees

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_get_configuration(const ds18b20_handle_t *const handle, ds18b20_config_t *const config) {
    if(!handle)
        return DS18B20_ERROR_HANDLE_NULL;
    
    if(!handle->is_init)
        return DS18B20_ERROR_NOT_INITIALIZED;
    
    if(!config)
        return DS18B20_ERROR_CONFIG_NULL;

    config->trigger_high = handle->scratchpad[2]; // reading data stored in a handle
    config->trigger_low = handle->scratchpad[3];
    config->resolution = handle->scratchpad[4];

    return DS18B20_OK;
}

const char *ds18b20_error_to_string(const ds18b20_error_t error) {
    return ds18b20_error_strings[error];
}

bool ds18b20_error_check(const ds18b20_error_t error, const char *const tag, const char *const message) {
    if(error) {
        if(message) {
            ESP_LOGE(tag, "%s%s", message, ds18b20_error_to_string(error));
        }
        else {
            ESP_LOGE(tag, "%s", ds18b20_error_to_string(error));
        }

        return true;
    }
    else {
        return false;
    }
}