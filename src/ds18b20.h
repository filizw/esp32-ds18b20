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

void ds18b20_reset(const ds18b20_handle_t *const handle);

void ds18b20_send_command(const ds18b20_handle_t *const handle, const uint8_t command);

void ds18b20_read_rom(ds18b20_handle_t *const handle);

void ds18b20_write_scratchpad(const ds18b20_handle_t *const handle, const uint8_t *const data);
void ds18b20_read_scratchpad(ds18b20_handle_t *const handle, const uint8_t num_of_bytes_to_read);

void ds18b20_init(ds18b20_handle_t *const handle, const ds18b20_config_t *const config);
void ds18b20_configure(ds18b20_handle_t *const handle, const ds18b20_config_t *const config);
void ds18b20_read_temperature(ds18b20_handle_t *const handle, float *const temperature);

void ds18b20_get_temperature(const ds18b20_handle_t *const handle, float *const temperature);
void ds18b20_get_configuration(const ds18b20_handle_t *const handle, ds18b20_config_t *const config);

#endif