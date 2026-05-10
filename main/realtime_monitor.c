#include "realtime_monitor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "gpio_manager.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "realtime_monitor";

static monitor_data_t monitor_data = {0};
static SemaphoreHandle_t monitor_mutex = NULL;
static esp_timer_handle_t update_timer = NULL;
static bool monitoring_enabled = false;

static void update_monitor_data(void *arg)
{
    if (!monitor_mutex || !monitoring_enabled) {
        return;
    }
    
    if (xSemaphoreTake(monitor_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        monitor_data.timestamp = esp_timer_get_time() / 1000; // milliseconds
        monitor_data.free_heap = esp_get_free_heap_size();
        monitor_data.min_free_heap = esp_get_minimum_free_heap_size();
        
        // Sample GPIO pins
        for (int i = 0; i < 16; i++) {
            monitor_data.gpio_levels[i] = gpio_get_level_safe(i);
        }
        
        // Calculate CPU usage (simplified)
        static uint32_t last_idle_time = 0;
        uint32_t current_idle_time = xTaskGetIdleTickCount();
        if (last_idle_time > 0) {
            uint32_t idle_delta = current_idle_time - last_idle_time;
            uint32_t total_delta = configTICK_RATE_HZ; // 1 second
            monitor_data.cpu_usage = 100 - ((idle_delta * 100) / total_delta);
            if (monitor_data.cpu_usage > 100) monitor_data.cpu_usage = 0;
        }
        last_idle_time = current_idle_time;
        
        // Update system uptime
        monitor_data.uptime = esp_timer_get_time() / 1000000;
        
        xSemaphoreGive(monitor_mutex);
    }
}

esp_err_t realtime_monitor_init(void)
{
    monitor_mutex = xSemaphoreCreateMutex();
    if (!monitor_mutex) {
        ESP_LOGE(TAG, "Failed to create monitor mutex");
        return ESP_ERR_NO_MEM;
    }
    
    esp_timer_create_args_t timer_args = {
        .callback = update_monitor_data,
        .name = "monitor_timer"
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &update_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create monitor timer: %s", esp_err_to_name(err));
        vSemaphoreDelete(monitor_mutex);
        return err;
    }
    
    ESP_LOGI(TAG, "Real-time monitor initialized");
    return ESP_OK;
}

esp_err_t realtime_monitor_start(uint32_t interval_ms)
{
    if (!update_timer || !monitor_mutex) {
        ESP_LOGE(TAG, "Monitor not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (monitoring_enabled) {
        ESP_LOGW(TAG, "Monitoring already enabled");
        return ESP_OK;
    }
    
    esp_err_t err = esp_timer_start_periodic(update_timer, interval_ms * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start monitor timer: %s", esp_err_to_name(err));
        return err;
    }
    
    monitoring_enabled = true;
    ESP_LOGI(TAG, "Real-time monitoring started with %d ms interval", interval_ms);
    return ESP_OK;
}

esp_err_t realtime_monitor_stop(void)
{
    if (!update_timer || !monitoring_enabled) {
        return ESP_OK;
    }
    
    esp_err_t err = esp_timer_stop(update_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop monitor timer: %s", esp_err_to_name(err));
        return err;
    }
    
    monitoring_enabled = false;
    ESP_LOGI(TAG, "Real-time monitoring stopped");
    return ESP_OK;
}

esp_err_t realtime_monitor_get_data(monitor_data_t *data)
{
    if (!data || !monitor_mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(monitor_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(data, &monitor_data, sizeof(monitor_data_t));
        xSemaphoreGive(monitor_mutex);
        return ESP_OK;
    }
    
    ESP_LOGW(TAG, "Failed to get monitor data - timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t realtime_monitor_get_json(char **json_output)
{
    monitor_data_t data;
    esp_err_t err = realtime_monitor_get_data(&data);
    if (err != ESP_OK) {
        return err;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "timestamp", data.timestamp);
    cJSON_AddNumberToObject(json, "uptime", data.uptime);
    cJSON_AddNumberToObject(json, "free_heap", data.free_heap);
    cJSON_AddNumberToObject(json, "min_free_heap", data.min_free_heap);
    cJSON_AddNumberToObject(json, "cpu_usage", data.cpu_usage);
    
    cJSON *gpio_array = cJSON_CreateArray();
    for (int i = 0; i < 16; i++) {
        cJSON *pin = cJSON_CreateObject();
        cJSON_AddNumberToObject(pin, "pin", i);
        cJSON_AddNumberToObject(pin, "level", data.gpio_levels[i]);
        cJSON_AddItemToArray(gpio_array, pin);
    }
    cJSON_AddItemToObject(json, "gpio", gpio_array);
    
    cJSON_AddBoolToObject(json, "monitoring_enabled", monitoring_enabled);
    
    *json_output = cJSON_Print(json);
    cJSON_Delete(json);
    
    return ESP_OK;
}

bool realtime_monitor_is_enabled(void)
{
    return monitoring_enabled;
}

void realtime_monitor_cleanup(void)
{
    if (update_timer) {
        esp_timer_stop(update_timer);
        esp_timer_delete(update_timer);
        update_timer = NULL;
    }
    
    if (monitor_mutex) {
        vSemaphoreDelete(monitor_mutex);
        monitor_mutex = NULL;
    }
    
    monitoring_enabled = false;
    memset(&monitor_data, 0, sizeof(monitor_data_t));
    
    ESP_LOGI(TAG, "Real-time monitor cleaned up");
}
