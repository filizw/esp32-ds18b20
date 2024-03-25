#ifndef DS18B20_H
#define DS18B20_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#define DS18B20_MAX_CONVERSION_TIME 750000

#define DS18B20_MAX_TEMPERATURE 0x7D
#define DS18B20_MIN_TEMPERATURE 0xC9

#define DS18B20_ROM_CODE_SIZE 8
#define DS18B20_SCRATCHPAD_SIZE 9



typedef enum {
    DS18B20_RESOLUTION_9_BIT = 0b00011111,
    DS18B20_RESOLUTION_10_BIT = 0b00111111,
    DS18B20_RESOLUTION_11_BIT = 0b01011111,
    DS18B20_RESOLUTION_12_BIT = 0b01111111
} ds18b20_resolution_t;

typedef enum {
    DS18B20_ERROR_UNKNOWN = -1,
    DS18B20_OK = 0,
    DS18B20_ERROR_INVALID_HANDLE,
    DS18B20_ERROR_INVALID_CONFIG,
    DS18B20_ERROR_INVALID_ARGUMENT,
    DS18B20_ERROR_RESET_FAILED
} ds18b20_error_t;

typedef struct {
    int8_t trigger_high;
    int8_t trigger_low;
    ds18b20_resolution_t resolution;
} ds18b20_config_t;

typedef struct {
    gpio_num_t gpio_pin;

    uint8_t rom_code[DS18B20_ROM_CODE_SIZE];
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];
} ds18b20_handle_t;

// initializes all communication with DS18B20
ds18b20_error_t ds18b20_reset(const ds18b20_handle_t *const handle);

// sends commands to communicate with DS18B20
ds18b20_error_t ds18b20_send_command(const ds18b20_handle_t *const handle, const uint8_t command);

// reads the DS18B20's 64-bit ROM, can only be used when there is one slave on the bus
ds18b20_error_t ds18b20_read_rom(ds18b20_handle_t *const handle);

// allows the master to write 3 bytes of data to the DS18B20’s scratchpad
ds18b20_error_t ds18b20_write_scratchpad(const ds18b20_handle_t *const handle, const uint8_t *const data);
// allows the master to read the contents of the scratchpad
ds18b20_error_t ds18b20_read_scratchpad(ds18b20_handle_t *const handle, const uint8_t num_of_bytes_to_read);

// initializes DS18B20
ds18b20_error_t ds18b20_init(ds18b20_handle_t *const handle, const ds18b20_config_t *const config);

// initializes DS18B20 with default settings
ds18b20_error_t ds18b20_init_default(ds18b20_handle_t *const handle);

// configures DS18B20's trigger low, trigger high values and resolution
ds18b20_error_t ds18b20_configure(ds18b20_handle_t *const handle, const ds18b20_config_t *const config);

// reads raw temperature from DS18B20 and stores it in handle
// if temperature argument is not NULL, it stores converted raw temperature value there
ds18b20_error_t ds18b20_read_temperature(ds18b20_handle_t *const handle, float *const temperature);

// converts raw temperature stored in handle and stores it in temperature argument
ds18b20_error_t ds18b20_get_temperature(const ds18b20_handle_t *const handle, float *const temperature);

// gets trigger low, trigger high and resolution values stored in the DS18B20
ds18b20_error_t ds18b20_get_configuration(const ds18b20_handle_t *const handle, ds18b20_config_t *const config);

#endif