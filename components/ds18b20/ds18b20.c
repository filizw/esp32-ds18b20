#include "ds18b20.h"

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

static void ds18b20_write_0(gpio_num_t owb_pin) {
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0); // pulling bus low for 60us
    ets_delay_us(60);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT); // releasing bus
    ets_delay_us(2);
}

static void ds18b20_write_1(gpio_num_t owb_pin) {
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0); // pulling bus low for 2us
    ets_delay_us(2);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT); // releasing bus
    ets_delay_us(60);
}

static uint8_t ds18b20_read_bit(gpio_num_t owb_pin) {
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0); // pulling bus low fow 2us
    ets_delay_us(2);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT); // releasing bus
    ets_delay_us(8);
    uint8_t bit = gpio_get_level(owb_pin); // storing value send by the DS18B20
    ets_delay_us(52);

    return bit;
}

static void ds18b20_write_byte(gpio_num_t owb_pin, uint8_t byte) {
    for(uint8_t i = 0; i < 8; i++) { // iterating through single bits and sending them to the DS18B20
        if(((byte >> i) & 1) == 0)
            ds18b20_write_0(owb_pin);
        else
            ds18b20_write_1(owb_pin);
    }
}

static uint8_t ds18b20_read_byte(gpio_num_t owb_pin) {
    uint8_t byte = 0;
    uint8_t bit = 0;

    for(uint8_t i = 0; i < 8; i++) { // reading one bit at a time and storing it in one byte
        bit = ds18b20_read_bit(owb_pin);
        byte |= (bit << i);
    }

    return byte;
}

static uint8_t ds18b20_check_crc(const uint8_t *const data, const uint8_t size) {
    if(data) {
        uint8_t crc = 0;

        for(uint8_t byte = 0; byte < size; byte++) { // calculating CRC from a CRC lookup table
            crc = ds18b20_crc_table[crc ^ data[byte]];
        }

        return crc; // if crc equals 0 then there is no error
    } else {
        ESP_LOGE(DS18B20_TAG, "check_crc: data argument is NULL");

        return 1;
    }
}

ds18b20_error_t ds18b20_reset(const ds18b20_handle_t *const handle) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "reset: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "reset: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }

    gpio_set_direction(handle->owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->owb_pin, 0); // reset pulse, pulling bus low for 480us
    ets_delay_us(480);
    gpio_set_direction(handle->owb_pin, GPIO_MODE_INPUT); // releasing bus

    ets_delay_us(70); // waiting for presense pulse
    const int presence_level = gpio_get_level(handle->owb_pin);

    ets_delay_us(410);
    const int recovery_level = gpio_get_level(handle->owb_pin);

    if(!presence_level && recovery_level) {
        return DS18B20_OK;
    }
    else {
        ESP_LOGE(DS18B20_TAG, "reset: reset failed, wrong response from slave device");

        return DS18B20_ERROR_RESET_FAILED;
    }
}

ds18b20_error_t ds18b20_send_command(const ds18b20_handle_t *const handle, uint8_t command) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "send_command: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "send_command: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }

    if(!DS18B20_COMMAND_IS_VALID(command)) {
        ESP_LOGE(DS18B20_TAG, "send_command: invalid command");

        return DS18B20_ERROR_INVALID_COMMAND;
    }

    ds18b20_write_byte(handle->owb_pin, command);

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_rom(ds18b20_handle_t *const handle) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "read_rom: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "read_rom: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }

    ds18b20_error_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_READ_ROM)))
        return err;

    for(uint8_t i = 0; i < DS18B20_ROM_CODE_SIZE; i++) { // reading one byte of the ROM code at a time
        handle->rom_code[i] = ds18b20_read_byte(handle->owb_pin);
    }

    if(handle->crc_enabled && ds18b20_check_crc(handle->rom_code, DS18B20_ROM_CODE_SIZE)) { // checking ROM code integrity
        ESP_LOGE(DS18B20_TAG, "read_rom: CRC failed");

        return DS18B20_ERROR_CRC;
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_write_scratchpad(const ds18b20_handle_t *const handle, const uint8_t *const data) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "write_scratchpad: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "write_scratchpad: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }
    
    if(!data) {
        ESP_LOGE(DS18B20_TAG, "write_scratchpad: data argument is NULL");

        return DS18B20_ERROR_ARGUMENT_NULL;
    }

    ds18b20_error_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_WRITE_SCRATCHPAD)))
        return err;

    ds18b20_write_byte(handle->owb_pin, data[0]); // writing to th register
    ds18b20_write_byte(handle->owb_pin, data[1]); // writing to tl register
    ds18b20_write_byte(handle->owb_pin, data[2]); // writing to configuration register

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_scratchpad(ds18b20_handle_t *const handle, uint8_t read_length) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "read_scratchpad: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "read_scratchpad: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }

    if(read_length < 1 && read_length > DS18B20_SCRATCHPAD_SIZE) {
        ESP_LOGE(DS18B20_TAG, "read_scratchpad: invalid value of read_length");

        return DS18B20_ERROR_INVALID_ARGUMENT;
    }

    ds18b20_error_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_READ_SCRATCHPAD)))
        return err;

    for(uint8_t i = 0; i < read_length; i++) { // reading one byte of the scratchpad at a time
        handle->scratchpad[i] = ds18b20_read_byte(handle->owb_pin);
    }

    if(read_length < DS18B20_SCRATCHPAD_SIZE) // if not reading the whole scratchpad then issue a reset
        ds18b20_reset(handle);
    
    if(handle->crc_enabled && ds18b20_check_crc(handle->scratchpad, DS18B20_SCRATCHPAD_SIZE)) { // checking scratchpad integrity
        ESP_LOGE(DS18B20_TAG, "read_scratchpad: CRC failed");

        return DS18B20_ERROR_CRC;
    }
    
    return DS18B20_OK;
}

ds18b20_error_t ds18b20_init(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "init: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }

    handle->is_init = true;
    
    if(!config) {
        handle->is_init = false;
        ESP_LOGE(DS18B20_TAG, "init: config is NULL");

        return DS18B20_ERROR_ARGUMENT_NULL;
    }

    if(!GPIO_IS_VALID_GPIO(config->owb_pin)) {
        handle->is_init = false;
        ESP_LOGE(DS18B20_TAG, "init: invalid gpio");

        return DS18B20_ERROR_INVALID_GPIO;
    }
    
    handle->owb_pin = config->owb_pin;
    handle->crc_enabled = config->enable_crc;

    ds18b20_error_t err;

    if((err = ds18b20_configure(handle, config))) { // send configuation to the DS18B20
        handle->is_init = false;

        return err;
    }
    
    if((err = ds18b20_read_rom(handle))) { // read ROM code and store it in a handle
        handle->is_init = false;

        return err;
    }
    
    return DS18B20_OK;
}

ds18b20_error_t ds18b20_init_default(ds18b20_handle_t *const handle, gpio_num_t owb_pin) {
    const ds18b20_config_t config = { // default settings
        .owb_pin = owb_pin,
        .enable_crc = DS18B20_CRC_DEFAULT,
        .trigger_high = DS18B20_TRIGGER_HIGH_DEFAULT,
        .trigger_low = DS18B20_TRIGGER_LOW_DEFAULT,
        .resolution = DS18B20_RESOLUTION_DEFAULT
    };

    return ds18b20_init(handle, &config);
}

ds18b20_error_t ds18b20_configure(ds18b20_handle_t *const handle, const ds18b20_config_t *const config) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "configure: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "configure: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }
    
    if(!config) {
        ESP_LOGE(DS18B20_TAG, "configure: config is NULL");

        return DS18B20_ERROR_ARGUMENT_NULL;
    }

    const uint8_t data[] = { // casting to the appropriate data type
        config->trigger_high,
        config->trigger_low,
        config->resolution
    };

    ds18b20_error_t err;

    if((err = ds18b20_write_scratchpad(handle, data))) // writing configuration to the DS18B20
        return err;
    
    if((err = ds18b20_read_scratchpad(handle, DS18B20_SCRATCHPAD_SIZE))) // reading written configuration
        return err;

    if((int8_t)handle->scratchpad[2] != config->trigger_high ||
       (int8_t)handle->scratchpad[3] != config->trigger_low ||
       (ds18b20_resolution_t)handle->scratchpad[4] != config->resolution) {
        ESP_LOGE(DS18B20_TAG, "configure: config between master and slave is not matching");

        return DS18B20_ERROR_CONFIGURATION_FAILED;
    }

    return DS18B20_OK;
}

ds18b20_error_t ds18b20_read_temperature(ds18b20_handle_t *const handle, float *const temperature) {
    if(!handle) {
        ESP_LOGE(DS18B20_TAG, "read_temperature: handle is NULL");

        return DS18B20_ERROR_HANDLE_NULL;
    }
    
    if(!handle->is_init) {
        ESP_LOGE(DS18B20_TAG, "read_temperature: handle is not initialized");

        return DS18B20_ERROR_NOT_INITIALIZED;
    }

    ds18b20_error_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_CONVERT)))
        return err;

    ets_delay_us(DS18B20_MAX_CONVERSION_TIME >> (3 - (handle->scratchpad[4] >> 5))); // calculating conversion time using a resolution value

    if((err = ds18b20_read_scratchpad(handle, (handle->crc_enabled ? DS18B20_SCRATCHPAD_SIZE : 2)))) // reading only a temperature value
        return err;
    
    if(temperature != NULL) {
        const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5)); // calculating mask that depends on a resolution
        *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f; // converting raw temperature to degrees
    }

    return DS18B20_OK;
}
