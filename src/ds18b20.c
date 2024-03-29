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
    "DS18B20_ERROR_NOT_INITIALIZED",
    "DS18B20_ERROR_CRC",
};

static const uint8_t ds18b20_crc_table[256] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
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

static uint8_t ds18b20_calculate_crc(const uint8_t *const data, const uint8_t size) {
    if(data) {
        uint8_t crc = 0;

        for(uint8_t byte = 0; byte < size; byte++) { // calculating CRC from a CRC lookup table
            crc = ds18b20_crc_table[crc ^ data[byte]];
        }

        return crc; // if crc equals 0 then there is no error
    } else {
        return 1;
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

    if(handle->crc_enabled && ds18b20_calculate_crc(handle->rom_code, DS18B20_ROM_CODE_SIZE)) // checking ROM code integrity
        return DS18B20_ERROR_CRC;

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
    
    if(handle->crc_enabled && ds18b20_calculate_crc(handle->scratchpad, DS18B20_SCRATCHPAD_SIZE)) // checking scratchpad integrity
        return DS18B20_ERROR_CRC;
    
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
        .enable_crc = DS18B20_CRC_DEFAULT,
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
        ESP_LOGE(tag ? tag : DS18B20_TAG, "%s%s", message ? message : "", ds18b20_error_to_string(error));

        return true;
    }
    else {
        return false;
    }
}
