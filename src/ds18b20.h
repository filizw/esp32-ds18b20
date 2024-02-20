#ifndef DS18B20_H
#define DS18B20_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

// ROM COMMANDS
#define DS18B20_SEARCH_ROM 0xF0
#define DS18B20_READ_ROM 0x33
#define DS18B20_MATCH_ROM 0x55
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_ALARM_SEARCH 0xEC

// FUNCTION COMMANDS
#define DS18B20_CONVERT 0x44
#define DS18B20_WRITE_SCRATCHPAD 0x4E
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_COPY_SCRATCHPAD 0x48
#define DS18B20_RECALL_E 0xB8
#define DS18B20_READ_POWER_SUPPLY 0xB4

typedef struct {
    gpio_num_t ow_pin;
} ds18b20_handler_t;

void ds18b20_init(const ds18b20_handler_t *const ds18b20_handler);

#endif