/*
 * ESP32 Small OS Simulation Header
 * 
 * Provides ESP32 API compatibility for simulation on PC.
 */

#ifndef SIM_ESP32_H
#define SIM_ESP32_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/* ESP32 Error Codes */
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_NO_MEM 2
#define ESP_ERR_TIMEOUT 3
#define ESP_ERR_INVALID_STATE 4
#define ESP_ERR_NOT_FOUND 5
#define ESP_ERR_INVALID_RESPONSE 6

/* ESP32 Log Functions */
#define ESP_LOGI(tag, ...) printf("[I][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGE(tag, ...) printf("[E][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGW(tag, ...) printf("[W][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGD(tag, ...) printf("[D][%s] " __VA_ARGS__), printf("\n")

/* ESP32 System Functions */
uint32_t esp_timer_get_time(void);
int esp_get_free_heap_size(void);
int esp_get_minimum_free_heap_size(void);
uint32_t esp_clk_cpu_freq(void);
void esp_restart(void);

/* ESP32 NVS Functions */
int nvs_flash_init(void);
int nvs_flash_erase(void);

/* ESP32 WiFi Functions */
int esp_wifi_init(void *config);
int esp_wifi_connect(void);
int esp_wifi_disconnect(void);
int esp_wifi_set_mode(int mode);
int esp_wifi_set_config(int interface, void *config);
int esp_wifi_start(void);
int esp_wifi_stop(void);

/* ESP32 Network Interface Functions */
int esp_netif_init(void);
void* esp_netif_create_default_wifi_sta(void);
void* esp_netif_get_handle_from_ifkey(const char *key);
int esp_netif_dhcpc_stop(void *netif);
int esp_netif_set_ip_info(void *netif, void *ip_info);

/* ESP32 Event Loop Functions */
typedef void* esp_event_base_t;
typedef int32_t esp_event_id_t;
typedef void (*esp_event_handler_t)(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

int esp_event_handler_register(esp_event_base_t event_base, esp_event_id_t event_id, 
                              esp_event_handler_t event_handler, void *arg);

/* ESP32 Timer Functions */
int esp_timer_init(void);

/* ESP32 HTTP Server Functions */
int http_server_init(void *config);
int http_server_start(void);

/* GPIO Simulation Functions */
int gpio_hal_init(void);
int gpio_hal_set_level(uint8_t pin, bool level);
bool gpio_hal_get_level(uint8_t pin);

/* Task Scheduler Simulation Functions */
void scheduler_init(void);
int scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                         uint32_t stack_size, int priority);
void scheduler_run_once(void);
int scheduler_delay(uint32_t ms);
int scheduler_yield(void);

/* Simulation Constants */
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WIFI_EVENT_WIFI_READY 0
#define WIFI_EVENT_STA_START 1
#define WIFI_EVENT_STA_CONNECTED 2
#define WIFI_EVENT_STA_DISCONNECTED 3
#define IP_EVENT 2
#define IP_EVENT_STA_GOT_IP 0

/* Simulation Macros */
#define ESP_ERROR_CHECK(x) do { \
    esp_err_t err = (x); \
    if (err != ESP_OK) { \
        ESP_LOGE("SIM", "Error at %s:%d: %d", __FILE__, __LINE__, err); \
        exit(1); \
    } \
} while(0)

#define pdMS_TO_TICKS(ms) (ms)
#define vTaskDelay(ms) usleep((ms) * 1000)

#endif /* SIM_ESP32_H */
