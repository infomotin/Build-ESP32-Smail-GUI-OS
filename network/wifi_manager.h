#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

// WiFi connection states
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECTING,
    WIFI_STATE_ERROR
} wifi_state_t;

// WiFi configuration
typedef struct {
    char ssid[32];
    char password[64];
    bool auto_reconnect;
    uint32_t reconnect_interval_ms;
    uint32_t connection_timeout_ms;
    uint32_t max_retry_attempts;
    bool enable_static_ip;
    esp_ip4_addr_t static_ip;
    esp_ip4_addr_t static_netmask;
    esp_ip4_addr_t static_gateway;
} wifi_config_t;

// WiFi statistics
typedef struct {
    wifi_state_t state;
    uint32_t connection_count;
    uint32_t failed_attempts;
    uint32_t reconnect_count;
    uint32_t total_uptime_ms;
    uint32_t current_uptime_ms;
    int8_t rssi;
    uint8_t channel;
    char mac_address[18];
    char ip_address[16];
    char gateway[16];
    char netmask[16];
    char dns_server[16];
    uint32_t last_connection_time;
    uint32_t last_disconnection_time;
} wifi_stats_t;

// WiFi scan result
typedef struct {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    wifi_auth_mode_t authmode;
    bool is_hidden;
    wifi_second_chan_t second_chan;
} wifi_scan_result_t;

// Function declarations
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_deinit(void);
esp_err_t wifi_manager_connect(const char *ssid, const char *password);
esp_err_t wifi_manager_disconnect(void);
esp_err_t wifi_manager_set_config(const wifi_config_t *config);
esp_err_t wifi_manager_get_config(wifi_config_t *config);
esp_err_t wifi_manager_start_scan(void);
esp_err_t wifi_manager_get_scan_results(wifi_scan_result_t *results, size_t max_results, size_t *found_count);
esp_err_t wifi_manager_get_stats(wifi_stats_t *stats);
wifi_state_t wifi_manager_get_state(void);
bool wifi_manager_is_connected(void);
esp_err_t wifi_manager_set_auto_reconnect(bool enable);
esp_err_t wifi_manager_save_config(void);
esp_err_t wifi_manager_load_config(void);

// Event callbacks
typedef void (*wifi_event_callback_t)(wifi_state_t state, void *arg);
esp_err_t wifi_manager_register_callback(wifi_event_callback_t callback, void *arg);
esp_err_t wifi_manager_unregister_callback(wifi_event_callback_t callback);

// Advanced functions
esp_err_t wifi_manager_set_power_save(bool enable);
esp_err_t wifi_manager_set_country_code(const char *country_code);
esp_err_t wifi_manager_set_bandwidth(wifi_bandwidth_t bandwidth);
esp_err_t wifi_manager_set_max_tx_power(int8_t power_dbm);
esp_err_t wifi_manager_get_ap_info(wifi_ap_record_t *ap_info);

// Debug functions
void wifi_manager_print_config(void);
void wifi_manager_print_stats(void);
esp_err_t wifi_manager_self_test(void);

// Constants
#define WIFI_MAX_SCAN_RESULTS 32
#define WIFI_DEFAULT_TIMEOUT_MS 10000
#define WIFI_DEFAULT_RETRY_ATTEMPTS 3
#define WIFI_DEFAULT_RECONNECT_INTERVAL_MS 5000
#define WIFI_MIN_RSSI -90
#define WIFI_MAX_RSSI 0
