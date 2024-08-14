#include "ds18b20.h"

#define WRITE_0_INIT_TIME 60
#define WRITE_0_RECOVERY_TIME 2

#define WRITE_1_INIT_TIME 2
#define WRITE_1_RECOVERY_TIME 60

#define READ_BIT_INIT_TIME 2
#define READ_BIT_SAMPLE_TIME 8
#define READ_BIT_RECOVERY_TIME 52

#define RESET_PULSE_TIME 480
#define PRESENCE_PULSE_SAMPLE_TIME 70
#define RESET_RECOVERY_TIME 410

#define COMMAND_IS_VALID(command) !((command ^ 0xF0) & (command ^ 0x33) & (command ^ 0x55) & (command ^ 0xCC) & \
                                    (command ^ 0xEC) & (command ^ 0x44) & (command ^ 0x4E) & (command ^ 0xBE) & \
                                    (command ^ 0x48) & (command ^ 0xB8) & (command ^ 0xB4))

struct ds18b20_t
{
    gpio_num_t owb_pin;
    uint8_t crc_enabled;

    uint8_t rom_code[DS18B20_ROM_CODE_SIZE];
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
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

static void ds18b20_write_0(gpio_num_t owb_pin)
{
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0);
    ets_delay_us(WRITE_0_INIT_TIME);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT);
    ets_delay_us(WRITE_0_RECOVERY_TIME);
}

static void ds18b20_write_1(gpio_num_t owb_pin)
{
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0);
    ets_delay_us(WRITE_1_INIT_TIME);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT);
    ets_delay_us(WRITE_1_RECOVERY_TIME);
}

static uint8_t ds18b20_read_bit(gpio_num_t owb_pin)
{
    gpio_set_direction(owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(owb_pin, 0);
    ets_delay_us(READ_BIT_INIT_TIME);
    gpio_set_direction(owb_pin, GPIO_MODE_INPUT);
    ets_delay_us(READ_BIT_SAMPLE_TIME);
    uint8_t bit = gpio_get_level(owb_pin);
    ets_delay_us(READ_BIT_RECOVERY_TIME);

    return bit;
}

static void ds18b20_write_byte(gpio_num_t owb_pin, uint8_t byte)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        if(((byte >> i) & 1) == 0)
            ds18b20_write_0(owb_pin);
        else
            ds18b20_write_1(owb_pin);
    }
}

static uint8_t ds18b20_read_byte(gpio_num_t owb_pin)
{
    uint8_t byte = 0;
    uint8_t bit = 0;

    for(uint8_t i = 0; i < 8; i++)
    {
        bit = ds18b20_read_bit(owb_pin);
        byte |= (bit << i);
    }

    return byte;
}

static uint8_t ds18b20_check_crc(const uint8_t *const data, const uint8_t size)
{
    uint8_t crc = 1;

    if(data != NULL)
    {
        crc = 0;
        
        for(uint8_t byte = 0; byte < size; byte++)
            crc = ds18b20_crc_table[crc ^ data[byte]];
    }

    return crc;
}

esp_err_t ds18b20_reset(const ds18b20_handle_t handle)
{
    if(handle == NULL)
        return ESP_ERR_INVALID_ARG;

    gpio_set_direction(handle->owb_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->owb_pin, 0);
    ets_delay_us(RESET_PULSE_TIME);
    gpio_set_direction(handle->owb_pin, GPIO_MODE_INPUT);

    ets_delay_us(PRESENCE_PULSE_SAMPLE_TIME);
    const int presence_level = gpio_get_level(handle->owb_pin);

    ets_delay_us(RESET_RECOVERY_TIME);
    const int recovery_level = gpio_get_level(handle->owb_pin);

    if(!presence_level && recovery_level)
        return ESP_OK;
    else
        return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t ds18b20_send_command(const ds18b20_handle_t handle, uint8_t command)
{
    if(handle == NULL || !COMMAND_IS_VALID(command))
        return ESP_ERR_INVALID_ARG;

    ds18b20_write_byte(handle->owb_pin, command);

    return ESP_OK;
}

esp_err_t ds18b20_read_rom(ds18b20_handle_t handle)
{
    if(handle == NULL)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_READ_ROM)))
        return err;

    for(uint8_t i = 0; i < DS18B20_ROM_CODE_SIZE; i++) // reading one byte of the ROM code at a time
        handle->rom_code[i] = ds18b20_read_byte(handle->owb_pin);

    if(handle->crc_enabled && ds18b20_check_crc(handle->rom_code, DS18B20_ROM_CODE_SIZE)) // checking the ROM code integrity
        return ESP_ERR_INVALID_CRC;

    return ESP_OK;
}

esp_err_t ds18b20_get_rom(ds18b20_handle_t handle, uint8_t *const buffer, uint8_t buf_size)
{
    if(handle == NULL || buffer == NULL)
        return ESP_ERR_INVALID_ARG;

    if(buf_size < 1 || buf_size > DS18B20_ROM_CODE_SIZE)
        return ESP_ERR_INVALID_SIZE;

    memcpy(buffer, handle->rom_code, buf_size);

    return ESP_OK;
}

esp_err_t ds18b20_write_scratchpad(const ds18b20_handle_t handle, const uint8_t *const data)
{
    if(handle == NULL || data == NULL)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_WRITE_SCRATCHPAD)))
        return err;

    ds18b20_write_byte(handle->owb_pin, data[0]); // writing to the th register
    ds18b20_write_byte(handle->owb_pin, data[1]); // writing to the tl register
    ds18b20_write_byte(handle->owb_pin, data[2]); // writing to the configuration register

    return ESP_OK;
}

esp_err_t ds18b20_read_scratchpad(ds18b20_handle_t handle, uint8_t read_length)
{
    if(handle == NULL)
        return ESP_ERR_INVALID_ARG;

    if(read_length < 1 || read_length > DS18B20_SCRATCHPAD_SIZE)
        return ESP_ERR_INVALID_SIZE;

    esp_err_t err;

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
    
    if(handle->crc_enabled && ds18b20_check_crc(handle->scratchpad, DS18B20_SCRATCHPAD_SIZE)) { // checking the scratchpad integrity
        return ESP_ERR_INVALID_CRC;
    }
    
    return ESP_OK;
}

esp_err_t ds18b20_get_scratchpad(ds18b20_handle_t handle, uint8_t *const buffer, uint8_t buf_size)
{
    if(handle == NULL || buffer == NULL)
        return ESP_ERR_INVALID_ARG;

    if(buf_size < 1 || buf_size > DS18B20_SCRATCHPAD_SIZE)
        return ESP_ERR_INVALID_SIZE;

    memcpy(buffer, handle->scratchpad, buf_size);

    return ESP_OK;
}

esp_err_t ds18b20_init(ds18b20_handle_t *handle, const ds18b20_config_t *const config)
{
    if(handle == NULL || config == NULL)
        return ESP_ERR_INVALID_ARG;
    
    *handle = malloc(sizeof(struct ds18b20_t));
    if(*handle == NULL)
        return ESP_ERR_NO_MEM;

    esp_err_t err = GPIO_IS_VALID_GPIO(config->owb_pin) ? ESP_OK : ESP_ERR_INVALID_ARG;
    
    (*handle)->owb_pin = config->owb_pin;
    (*handle)->crc_enabled = config->enable_crc;

    const uint8_t data[] = {
        config->trigger_high,
        config->trigger_low,
        config->resolution
    };

    if(err == ESP_OK)
        err = ds18b20_write_scratchpad(*handle, data);
    
    if(err == ESP_OK)
        err = ds18b20_read_scratchpad(*handle, DS18B20_SCRATCHPAD_SIZE);

    if((err == ESP_OK) &&
       ((int8_t)(*handle)->scratchpad[2] != config->trigger_high ||
        (int8_t)(*handle)->scratchpad[3] != config->trigger_low ||
        (ds18b20_resolution_t)(*handle)->scratchpad[4] != config->resolution))
        err = ESP_FAIL;
    
    if(err == ESP_OK)
        err = ds18b20_read_rom(*handle);
    
    if(err != ESP_OK)
        free(*handle);
    
    return err;
}

esp_err_t ds18b20_init_default(ds18b20_handle_t *handle, const gpio_num_t owb_pin)
{
    if(handle == NULL || !GPIO_IS_VALID_GPIO(owb_pin))
        return ESP_ERR_INVALID_ARG;
    
    *handle = malloc(sizeof(struct ds18b20_t));
    if(*handle == NULL)
        return ESP_ERR_NO_MEM;

    (*handle)->owb_pin = owb_pin;
    (*handle)->crc_enabled = DS18B20_CRC_DEFAULT;
    
    esp_err_t err = ds18b20_read_scratchpad(*handle, DS18B20_SCRATCHPAD_SIZE);
    
    if(err == ESP_OK)
        err = ds18b20_read_rom(*handle);
    
    if(err != ESP_OK)
        free(*handle);
    
    return err;
}

esp_err_t ds18b20_deinit(ds18b20_handle_t *handle)
{
    if(handle == NULL || *handle == NULL)
        return ESP_ERR_INVALID_ARG;
    
    free(*handle);
    *handle = NULL;

    return ESP_OK;
}

esp_err_t ds18b20_read_temperature(ds18b20_handle_t handle, float *const temperature)
{
    if(handle == NULL)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err;

    if((err = ds18b20_reset(handle)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_SKIP_ROM)))
        return err;
    
    if((err = ds18b20_send_command(handle, DS18B20_COMMAND_CONVERT)))
        return err;

    ets_delay_us(DS18B20_MAX_CONVERSION_TIME >> (3 - (handle->scratchpad[4] >> 5))); // calculating the conversion time using a resolution value

    if((err = ds18b20_read_scratchpad(handle, (handle->crc_enabled ? DS18B20_SCRATCHPAD_SIZE : 2)))) // reading only the temperature value
        return err;
    
    if(temperature != NULL)
    {
        const uint8_t mask = 0xff << (3 - (handle->scratchpad[4] >> 5)); // calculating the mask that depends on a resolution
        *temperature = ((handle->scratchpad[1] << 8) | (handle->scratchpad[0] & mask)) / 16.0f; // converting the raw temperature to degrees
    }

    return ESP_OK;
}
