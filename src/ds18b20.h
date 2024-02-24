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

#define DS18B20_SCRATCHPAD_SIZE 9

typedef enum {
    DS18B20_RESOLUTION_9_BIT,
    DS18B20_RESOLUTION_10_BIT,
    DS18B20_RESOLUTION_11_BIT,
    DS18B20_RESOLUTION_12_BIT
} ds18b20_resolution_t;

typedef struct {
    gpio_num_t ow_pin;
    ds18b20_resolution_t resolution;
} ds18b20_handler_t;

typedef struct {
    uint8_t trigger_high;
    uint8_t trigger_low;
    ds18b20_resolution_t resolution;
} ds18b20_config_t;

void ds18b20_reset(const ds18b20_handler_t *const ds18b20_handler);
void ds18b20_send_command(const ds18b20_handler_t *const ds18b20_handler, const uint8_t command);
void ds18b20_write_scratchpad(ds18b20_handler_t *const ds18b20_handler, const uint8_t *const data);
void ds18b20_read_scratchpad(const ds18b20_handler_t *const ds18b20_handler, uint8_t *const buffer, const uint8_t buffer_size);
void ds18b20_configure(ds18b20_handler_t *const ds18b20_handler, const ds18b20_config_t *const ds18b20_config);
void ds18b20_read_temperature(const ds18b20_handler_t *const ds18b20_handler, float *const temperature);

#endif