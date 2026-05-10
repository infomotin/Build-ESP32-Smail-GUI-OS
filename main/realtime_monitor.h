#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t timestamp;        // Current timestamp in milliseconds
    uint32_t uptime;           // System uptime in seconds
    uint32_t free_heap;        // Free heap size in bytes
    uint32_t min_free_heap;    // Minimum free heap size
    uint8_t gpio_levels[16];   // GPIO pin levels 0-15
    uint8_t cpu_usage;         // CPU usage percentage (0-100)
} monitor_data_t;

esp_err_t realtime_monitor_init(void);
esp_err_t realtime_monitor_start(uint32_t interval_ms);
esp_err_t realtime_monitor_stop(void);
esp_err_t realtime_monitor_get_data(monitor_data_t *data);
esp_err_t realtime_monitor_get_json(char **json_output);
bool realtime_monitor_is_enabled(void);
void realtime_monitor_cleanup(void);
