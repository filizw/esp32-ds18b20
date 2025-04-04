#ifndef DS18B20_H
#define DS18B20_H

#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

// ROM COMMANDS
#define DS18B20_COMMAND_SEARCH_ROM 0xF0
#define DS18B20_COMMAND_READ_ROM 0x33
#define DS18B20_COMMAND_MATCH_ROM 0x55
#define DS18B20_COMMAND_SKIP_ROM 0xCC
#define DS18B20_COMMAND_ALARM_SEARCH 0xEC

// FUNCTION COMMANDS
#define DS18B20_COMMAND_CONVERT 0x44
#define DS18B20_COMMAND_WRITE_SCRATCHPAD 0x4E
#define DS18B20_COMMAND_READ_SCRATCHPAD 0xBE
#define DS18B20_COMMAND_COPY_SCRATCHPAD 0x48
#define DS18B20_COMMAND_RECALL_E 0xB8
#define DS18B20_COMMAND_READ_POWER_SUPPLY 0xB4

// memory sizes
#define DS18B20_ROM_CODE_SIZE 8
#define DS18B20_SCRATCHPAD_SIZE 9

// CRC settings
#define DS18B20_CRC_ENABLE 1
#define DS18B20_CRC_DISABLE 0
#define DS18B20_CRC_DEFAULT DS18B20_CRC_DISABLE

// default trigger settings
#define DS18B20_TRIGGER_HIGH_DEFAULT 0x7F
#define DS18B20_TRIGGER_LOW_DEFAULT 0x80

// maximum temperature conversion time
#define DS18B20_MAX_CONVERSION_TIME 750000

// resolution settings
typedef enum
{
    DS18B20_RESOLUTION_9_BIT = 0x1F,
    DS18B20_RESOLUTION_10_BIT = 0x3F,
    DS18B20_RESOLUTION_11_BIT = 0x2F,
    DS18B20_RESOLUTION_12_BIT = 0x7F,
    DS18B20_RESOLUTION_DEFAULT = DS18B20_RESOLUTION_12_BIT
} ds18b20_resolution_t;

typedef struct ds18b20_t *ds18b20_handle_t;

typedef struct
{
    gpio_num_t owb_pin;
    uint8_t enable_crc;

    int8_t trigger_high;
    int8_t trigger_low;
    ds18b20_resolution_t resolution;
} ds18b20_config_t;

// initializes all communication with the DS18B20
esp_err_t ds18b20_reset(const ds18b20_handle_t handle);

// sends commands to communicate with the DS18B20
esp_err_t ds18b20_send_command(const ds18b20_handle_t handle, uint8_t command);

// reads the DS18B20's 64-bit ROM, can only be used when there is one slave on the bus
esp_err_t ds18b20_read_rom(ds18b20_handle_t handle);

// writes the ROM code stored in the handle to the buffer
esp_err_t ds18b20_get_rom(ds18b20_handle_t handle, uint8_t *const buffer, uint8_t buf_size);

// allows the master to write 3 bytes of data to the DS18B20’s scratchpad
esp_err_t ds18b20_write_scratchpad(const ds18b20_handle_t handle, const uint8_t *const data);

// allows the master to read the contents of the scratchpad
esp_err_t ds18b20_read_scratchpad(ds18b20_handle_t handle, uint8_t read_length);

// writes the scratchpad stored in the handle to the buffer
esp_err_t ds18b20_get_scratchpad(ds18b20_handle_t handle, uint8_t *const buffer, uint8_t buf_size);

// initializes the DS18B20
esp_err_t ds18b20_init(ds18b20_handle_t *handle, const ds18b20_config_t *const config);

// initializes the DS18B20 with default settings
esp_err_t ds18b20_init_default(ds18b20_handle_t *handle, const gpio_num_t owb_pin);

// deinitializes the DS18B20
esp_err_t ds18b20_deinit(ds18b20_handle_t *handle);

// reads raw temperature from the DS18B20 and stores it in the handle
// if the temperature argument is not NULL, it stores the converted to degrees temperature value in that argument
esp_err_t ds18b20_read_temperature(ds18b20_handle_t handle, float *const temperature);

#endif
