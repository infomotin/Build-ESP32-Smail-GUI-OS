#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

#define TAG "WIFI_MANAGER"

// Global variables
static wifi_config_t wifi_config;
static wifi_stats_t wifi_stats;
static wifi_state_t current_state = WIFI_STATE_DISCONNECTED;
static esp_netif_t *sta_netif = NULL;
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_got_ip = NULL;
static SemaphoreHandle_t wifi_mutex = NULL;
static TaskHandle_t wifi_task_handle = NULL;
static wifi_event_callback_t event_callback = NULL;
static void *event_callback_arg = NULL;
static wifi_scan_result_t scan_results[WIFI_MAX_SCAN_RESULTS];
static size_t scan_result_count = 0;
static bool scan_in_progress = false;

// Forward declarations
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_task(void *arg);
static void update_wifi_stats(void);
static esp_err_t configure_wifi_station(void);
static void reset_wifi_stats(void);

// WiFi event handler
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                current_state = WIFI_STATE_CONNECTING;
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi connected to AP");
                current_state = WIFI_STATE_CONNECTED;
                wifi_stats.connection_count++;
                wifi_stats.last_connection_time = esp_timer_get_time() / 1000;
                update_wifi_stats();
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
                    ESP_LOGW(TAG, "WiFi disconnected: %d", disconnected->reason);
                    current_state = WIFI_STATE_DISCONNECTED;
                    wifi_stats.last_disconnection_time = esp_timer_get_time() / 1000;
                    wifi_stats.failed_attempts++;
                    
                    // Auto-reconnect if enabled
                    if (wifi_config.auto_reconnect && wifi_config.max_retry_attempts > 0) {
                        current_state = WIFI_STATE_RECONNECTING;
                        ESP_LOGI(TAG, "Will attempt to reconnect in %d ms", wifi_config.reconnect_interval_ms);
                    }
                    break;
                }
                
            case WIFI_EVENT_STA_AUTHMODE_CHANGE:
                ESP_LOGI(TAG, "WiFi auth mode changed");
                break;
                
            case WIFI_EVENT_STA_WPS_ER_SUCCESS:
                ESP_LOGI(TAG, "WiFi WPS succeeded");
                break;
                
            case WIFI_EVENT_STA_WPS_ER_FAILED:
                ESP_LOGE(TAG, "WiFi WPS failed");
                break;
                
            default:
                ESP_LOGD(TAG, "Unhandled WiFi event: %d", event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
            strcpy(wifi_stats.ip_address, inet_ntoa(event->ip_info.ip));
            strcpy(wifi_stats.gateway, inet_ntoa(event->ip_info.gw));
            strcpy(wifi_stats.netmask, inet_ntoa(event->ip_info.netmask));
        } else if (event_id == IP_EVENT_STA_LOST_IP) {
            ESP_LOGW(TAG, "Lost IP address");
            strcpy(wifi_stats.ip_address, "0.0.0.0");
        }
    }
    
    // Call user callback if registered
    if (event_callback) {
        event_callback(current_state, event_callback_arg);
    }
}

// WiFi task
static void wifi_task(void *arg)
{
    ESP_LOGI(TAG, "WiFi management task started");
    
    while (1) {
        if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Handle reconnection logic
            if (current_state == WIFI_STATE_RECONNECTING && wifi_config.auto_reconnect) {
                static uint32_t last_reconnect_attempt = 0;
                uint32_t current_time = esp_timer_get_time() / 1000;
                
                if (current_time - last_reconnect_attempt >= wifi_config.reconnect_interval_ms) {
                    ESP_LOGI(TAG, "Attempting to reconnect to WiFi");
                    
                    esp_err_t err = esp_wifi_connect();
                    if (err == ESP_OK) {
                        wifi_stats.reconnect_count++;
                        last_reconnect_attempt = current_time;
                        current_state = WIFI_STATE_CONNECTING;
                    } else {
                        ESP_LOGE(TAG, "Failed to start WiFi connection: %s", esp_err_to_name(err));
                    }
                }
            }
            
            // Update uptime
            if (current_state == WIFI_STATE_CONNECTED) {
                wifi_stats.current_uptime_ms += 100; // Task runs every 100ms
                wifi_stats.total_uptime_ms += 100;
            }
            
            xSemaphoreGive(wifi_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Update WiFi statistics
static void update_wifi_stats(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK) {
        wifi_stats.rssi = ap_info.rssi;
        wifi_stats.channel = ap_info.primary;
        memcpy(wifi_stats.mac_address, ap_info.bssid, 6);
        
        // Format MAC address
        snprintf(wifi_stats.mac_address, sizeof(wifi_stats.mac_address),
                "%02X:%02X:%02X:%02X:%02X:%02X",
                ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
    }
}

// Configure WiFi station
static esp_err_t configure_wifi_station(void)
{
    esp_err_t err;
    
    // Configure WiFi
    wifi_config_t wifi_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    strncpy((char *)wifi_cfg.sta.ssid, wifi_config.ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, wifi_config.password, sizeof(wifi_cfg.sta.password) - 1);
    
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure static IP if enabled
    if (wifi_config.enable_static_ip) {
        esp_netif_dhcp_status_t dhcp_status;
        esp_netif_get_dhcp_status(sta_netif, &dhcp_status);
        
        if (dhcp_status == ESP_NETIF_DHCP_STARTED) {
            esp_err_t err = esp_netif_dhcpc_stop(sta_netif);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to stop DHCP: %s", esp_err_to_name(err));
                return err;
            }
        }
        
        esp_netif_ip_info_t ip_info;
        ip_info.ip.addr = esp_ip4_addr_u32(&wifi_config.static_ip);
        ip_info.netmask.addr = esp_ip4_addr_u32(&wifi_config.static_netmask);
        ip_info.gw.addr = esp_ip4_addr_u32(&wifi_config.static_gateway);
        
        err = esp_netif_set_ip_info(sta_netif, &ip_info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set static IP: %s", esp_err_to_name(err));
            return err;
        }
    }
    
    return ESP_OK;
}

// Reset WiFi statistics
static void reset_wifi_stats(void)
{
    memset(&wifi_stats, 0, sizeof(wifi_stats_t));
    wifi_stats.state = WIFI_STATE_DISCONNECTED;
    strcpy(wifi_stats.ip_address, "0.0.0.0");
    strcpy(wifi_stats.gateway, "0.0.0.0");
    strcpy(wifi_stats.netmask, "0.0.0.0");
}

// Initialize WiFi manager
esp_err_t wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi manager");
    
    // Initialize mutex
    wifi_mutex = xSemaphoreCreateMutex();
    if (!wifi_mutex) {
        ESP_LOGE(TAG, "Failed to create WiFi mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Reset statistics
    reset_wifi_stats();
    
    // Set default configuration
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    wifi_config.auto_reconnect = true;
    wifi_config.reconnect_interval_ms = WIFI_DEFAULT_RECONNECT_INTERVAL_MS;
    wifi_config.connection_timeout_ms = WIFI_DEFAULT_TIMEOUT_MS;
    wifi_config.max_retry_attempts = WIFI_DEFAULT_RETRY_ATTEMPTS;
    
    // Initialize TCP/IP stack
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(err));
        return err;
    }
    
    // Create default WiFi STA netif
    err = esp_netif_create_default_wifi_sta();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA netif: %s", esp_err_to_name(err));
        return err;
    }
    
    // Get the netif handle
    sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta_netif) {
        ESP_LOGE(TAG, "Failed to get WiFi STA netif handle");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Initialize WiFi
    err = esp_wifi_init(&wifi_init_config_default());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(err));
        return err;
    }
    
    // Set WiFi mode
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(err));
        return err;
    }
    
    // Register event handlers
    err = esp_event_handler_instance_register(WIFI_EVENT,
                                                ESP_EVENT_ANY_ID,
                                                &wifi_event_handler,
                                                NULL,
                                                &instance_any_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(err));
        return err;
    }
    
    err = esp_event_handler_instance_register(IP_EVENT,
                                                IP_EVENT_STA_GOT_IP,
                                                &wifi_event_handler,
                                                NULL,
                                                &instance_got_ip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(err));
        return err;
    }
    
    // Create WiFi management task
    BaseType_t ret = xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, &wifi_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WiFi task");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    return ESP_OK;
}

// Connect to WiFi
esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    if (!ssid || !wifi_mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = ESP_OK;
    
    // Store credentials
    strncpy(wifi_config.ssid, ssid, sizeof(wifi_config.ssid) - 1);
    wifi_config.ssid[sizeof(wifi_config.ssid) - 1] = '\0';
    
    if (password) {
        strncpy(wifi_config.password, password, sizeof(wifi_config.password) - 1);
        wifi_config.password[sizeof(wifi_config.password) - 1] = '\0';
    } else {
        wifi_config.password[0] = '\0';
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", wifi_config.ssid);
    
    // Configure WiFi station
    err = configure_wifi_station();
    if (err != ESP_OK) {
        xSemaphoreGive(wifi_mutex);
        return err;
    }
    
    // Start WiFi
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(err));
        xSemaphoreGive(wifi_mutex);
        return err;
    }
    
    // Connect to AP
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s", esp_err_to_name(err));
        xSemaphoreGive(wifi_mutex);
        return err;
    }
    
    current_state = WIFI_STATE_CONNECTING;
    xSemaphoreGive(wifi_mutex);
    
    ESP_LOGI(TAG, "WiFi connection initiated");
    return ESP_OK;
}

// Disconnect from WiFi
esp_err_t wifi_manager_disconnect(void)
{
    if (!wifi_mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    esp_err_t err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect from WiFi: %s", esp_err_to_name(err));
        xSemaphoreGive(wifi_mutex);
        return err;
    }
    
    current_state = WIFI_STATE_DISCONNECTED;
    xSemaphoreGive(wifi_mutex);
    
    ESP_LOGI(TAG, "WiFi disconnected");
    return ESP_OK;
}

// Get WiFi statistics
esp_err_t wifi_manager_get_stats(wifi_stats_t *stats)
{
    if (!stats || !wifi_mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    *stats = wifi_stats;
    stats->state = current_state;
    
    xSemaphoreGive(wifi_mutex);
    return ESP_OK;
}

// Get WiFi state
wifi_state_t wifi_manager_get_state(void)
{
    return current_state;
}

// Check if WiFi is connected
bool wifi_manager_is_connected(void)
{
    return current_state == WIFI_STATE_CONNECTED;
}

// Set auto reconnect
esp_err_t wifi_manager_set_auto_reconnect(bool enable)
{
    if (!wifi_mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    wifi_config.auto_reconnect = enable;
    
    xSemaphoreGive(wifi_mutex);
    return ESP_OK;
}

// Register event callback
esp_err_t wifi_manager_register_callback(wifi_event_callback_t callback, void *arg)
{
    if (!callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    event_callback = callback;
    event_callback_arg = arg;
    
    return ESP_OK;
}

// Unregister event callback
esp_err_t wifi_manager_unregister_callback(wifi_event_callback_t callback)
{
    if (event_callback == callback) {
        event_callback = NULL;
        event_callback_arg = NULL;
    }
    
    return ESP_OK;
}

// Start WiFi scan
esp_err_t wifi_manager_start_scan(void)
{
    if (scan_in_progress || !wifi_mutex) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    scan_in_progress = true;
    scan_result_count = 0;
    
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 120,
        .scan_time.active.max = 120,
    };
    
    esp_err_t err = esp_wifi_scan_start(&scan_config, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(err));
        scan_in_progress = false;
        xSemaphoreGive(wifi_mutex);
        return err;
    }
    
    xSemaphoreGive(wifi_mutex);
    
    ESP_LOGI(TAG, "WiFi scan started");
    return ESP_OK;
}

// Get scan results
esp_err_t wifi_manager_get_scan_results(wifi_scan_result_t *results, size_t max_results, size_t *found_count)
{
    if (!results || !found_count || !wifi_mutex) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    size_t count = (scan_result_count < max_results) ? scan_result_count : max_results;
    memcpy(results, scan_results, count * sizeof(wifi_scan_result_t));
    *found_count = count;
    
    xSemaphoreGive(wifi_mutex);
    return ESP_OK;
}

// Print WiFi configuration
void wifi_manager_print_config(void)
{
    ESP_LOGI(TAG, "=== WiFi Configuration ===");
    ESP_LOGI(TAG, "SSID: %s", wifi_config.ssid);
    ESP_LOGI(TAG, "Password: %s", strlen(wifi_config.password) > 0 ? "***" : "none");
    ESP_LOGI(TAG, "Auto reconnect: %s", wifi_config.auto_reconnect ? "enabled" : "disabled");
    ESP_LOGI(TAG, "Reconnect interval: %d ms", wifi_config.reconnect_interval_ms);
    ESP_LOGI(TAG, "Connection timeout: %d ms", wifi_config.connection_timeout_ms);
    ESP_LOGI(TAG, "Max retry attempts: %d", wifi_config.max_retry_attempts);
    ESP_LOGI(TAG, "Static IP: %s", wifi_config.enable_static_ip ? "enabled" : "disabled");
    if (wifi_config.enable_static_ip) {
        ESP_LOGI(TAG, "Static IP: " IPSTR, IP2STR(&wifi_config.static_ip));
        ESP_LOGI(TAG, "Static netmask: " IPSTR, IP2STR(&wifi_config.static_netmask));
        ESP_LOGI(TAG, "Static gateway: " IPSTR, IP2STR(&wifi_config.static_gateway));
    }
    ESP_LOGI(TAG, "========================");
}

// Print WiFi statistics
void wifi_manager_print_stats(void)
{
    wifi_stats_t stats;
    if (wifi_manager_get_stats(&stats) == ESP_OK) {
        ESP_LOGI(TAG, "=== WiFi Statistics ===");
        ESP_LOGI(TAG, "State: %s", 
                  stats.state == WIFI_STATE_CONNECTED ? "Connected" :
                  stats.state == WIFI_STATE_CONNECTING ? "Connecting" :
                  stats.state == WIFI_STATE_RECONNECTING ? "Reconnecting" :
                  stats.state == WIFI_STATE_DISCONNECTED ? "Disconnected" : "Unknown");
        ESP_LOGI(TAG, "IP Address: %s", stats.ip_address);
        ESP_LOGI(TAG, "MAC Address: %s", stats.mac_address);
        ESP_LOGI(TAG, "RSSI: %d dBm", stats.rssi);
        ESP_LOGI(TAG, "Channel: %d", stats.channel);
        ESP_LOGI(TAG, "Connection count: %d", stats.connection_count);
        ESP_LOGI(TAG, "Failed attempts: %d", stats.failed_attempts);
        ESP_LOGI(TAG, "Reconnect count: %d", stats.reconnect_count);
        ESP_LOGI(TAG, "Current uptime: %d ms", stats.current_uptime_ms);
        ESP_LOGI(TAG, "Total uptime: %d ms", stats.total_uptime_ms);
        ESP_LOGI(TAG, "=======================");
    }
}

// WiFi self test
esp_err_t wifi_manager_self_test(void)
{
    ESP_LOGI(TAG, "Starting WiFi manager self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test initialization
    if (!wifi_mutex || !sta_netif) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        result = ESP_ERR_INVALID_STATE;
    }
    
    // Test state functions
    wifi_state_t state = wifi_manager_get_state();
    ESP_LOGI(TAG, "Current WiFi state: %d", state);
    
    bool connected = wifi_manager_is_connected();
    ESP_LOGI(TAG, "WiFi connected: %s", connected ? "yes" : "no");
    
    // Test statistics
    wifi_stats_t stats;
    esp_err_t err = wifi_manager_get_stats(&stats);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi statistics: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "WiFi manager self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
