/*
 * Small OS for ESP32 - Core System
 * System architecture:
 *
 * +---------------------+
 * |    Web Dashboard     |  HTML/CSS/JS served from ESP32
 * |   (User Interface)   |  Communicates via REST API + WebSocket
 * +----------+----------+
 *            | HTTP/WebSocket
 * +----------v----------+
 * |    HTTP Server       |  Lightweight httpd (ESP-IDF)
 * |    + REST API        |  /api/* endpoints
 * +----------+----------+
 *            | Events
 * +----------v----------+
 * |   Activity Manager   |  Stores & executes activity sequences
 * |   + Scheduler        |  Timer-based and event-based triggers
 * +----------+----------+
 *            | Commands
 * +----------v----------+   +------------------+
 * |    GPIO Manager      |   |  Storage (NVS)   |
 * |  Digital/PWM/ADC     +---+  Config & Data   |
 * +----------+----------+   +------------------+
 *            |
 * +----------v----------+
 * |  Network Manager     |  WiFi AP + STA mode
 * |  + Auth              |  Token-based auth
 * +----------+----------+
 *            |
 * +----------v----------+
 * |  ESP32 Hardware      |  CPU0: Main loop + scheduler
 * |  (FreeRTOS dual-core)|  CPU1: WiFi + HTTP server tasks
 * +---------------------+
 *
 * Memory Layout (ESP32 with 520KB SRAM, 4MB Flash):
 * ┌─────────────────────────────────────────────┐
 * │  Flash (4MB)                                │
 * ├─────────────────────────────────────────────┤
 * │  App Partition     (1.6MB)                  │
 * │  - Kernel/OS code  (~200KB)                 │
 * │  - Drivers         (~100KB)                 │
 * │  - Web server      (~150KB)                 │
 * │  - Web assets (SPIFFS)(~200KB)              │
 * │  - NVS Partition   (~1MB)                   │
 * ├─────────────────────────────────────────────┤
 * │  SRAM (520KB)                               │
 * ├─────────────────────────────────────────────┤
 * │  FreeRTOS Kernel  (~30KB)                   │
 * │  TCP/IP Stack     (~60KB)  [Core 0]         │
 * │  HTTP Server      (~20KB)  [Core 1]         │
 * │  App Tasks        (~80KB)                   │
 * │  GPIO Buffers     (~8KB)                    │
 * │  Web Resources    (~50KB)                   │
 * │  System Heap      (~200KB free)             │
 * └─────────────────────────────────────────────┘
 */

#include "small_os.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "small_os";

/* System uptime tracking */
static uint32_t boot_timestamp = 0;

/* Module initialization status */
static bool modules_initialized[EVENT_MAX] = {false};

/* System status cache (updated periodically) */
static system_status_t g_status;

const char *small_os_get_version(void)
{
    return SMALL_OS_VERSION;
}

uint32_t small_os_get_uptime(void)
{
    if (boot_timestamp == 0) {
        return 0;
    }
    return (esp_timer_get_time() / 1000000ULL) - boot_timestamp;
}

void small_os_mark_module_ready(event_type_t module)
{
    if (module >= 0 && module < EVENT_MAX) {
        modules_initialized[module] = true;
    }
}

bool small_os_is_module_ready(event_type_t module)
{
    if (module >= 0 && module < EVENT_MAX) {
        return modules_initialized[module];
    }
    return false;
}

const char *small_os_event_type_str(event_type_t type)
{
    static const char *names[] = {
        "NONE", "TIMER", "ACTIVITY", "API",
        "GPIO_CHANGE", "WIFI_CONNECTED", "WIFI_DISCONNECTED"
    };
    if (type >= EVENT_NONE && type < EVENT_MAX) {
        return names[type];
    }
    return "UNKNOWN";
}

void system_status_update(system_status_t *status)
{
    if (status == NULL) return;

    memset(status, 0, sizeof(system_status_t));
    status->uptime_seconds = small_os_get_uptime();
    status->free_heap = esp_get_free_heap_size();
    status->min_free_heap = esp_get_minimum_free_heap_size();
    status->total_heap = esp_get_free_heap_size() + esp_get_minimum_free_heap_size();
    snprintf(status->version, sizeof(status->version), "%s", SMALL_OS_VERSION);

    /* Device name from NVS */
    const char *name = storage_get_config("device_name");
    if (name) {
        snprintf(status->device_name, sizeof(status->device_name), "%s", name);
    } else {
        snprintf(status->device_name, sizeof(status->device_name), "SmallOS-ESP32");
    }

    /* IP address from network */
    const char *ip = network_manager_get_ip();
    if (ip) {
        snprintf(status->ip_address, sizeof(status->ip_address), "%s", ip);
    } else {
        snprintf(status->ip_address, sizeof(status->ip_address), "0.0.0.0");
    }

    /* MAC address */
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(status->mac_address, sizeof(status->mac_address),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    status->wifi_status = network_manager_get_wifi_state();
}

void system_status_debug_print(void)
{
    system_status_update(&g_status);
    ESP_LOGI(TAG, "====== System Status ======");
    ESP_LOGI(TAG, "Device: %s", g_status.device_name);
    ESP_LOGI(TAG, "Version: %s", g_status.version);
    ESP_LOGI(TAG, "Uptime: %lu seconds", g_status.uptime_seconds);
    ESP_LOGI(TAG, "Free Heap: %lu bytes", g_status.free_heap);
    ESP_LOGI(TAG, "Min Free Heap: %lu bytes", g_status.min_free_heap);
    ESP_LOGI(TAG, "IP Address: %s", g_status.ip_address);
    ESP_LOGI(TAG, "MAC Address: %s", g_status.mac_address);
    ESP_LOGI(TAG, "WiFi Status: %lu", g_status.wifi_status);
    ESP_LOGI(TAG, "==============================");
}

void small_os_init(void)
{
    boot_timestamp = esp_timer_get_time() / 1000000ULL;
    memset(&g_status, 0, sizeof(g_status));

    /* Initialize modules in dependency order */
    ESP_LOGI(TAG, "Initializing Small OS v%s", SMALL_OS_VERSION);

    /* Storage first - others depend on it */
    storage_init();
    small_os_mark_module_ready(EVENT_NONE);

    /* GPIO */
    gpio_manager_init();

    /* Network */
    network_manager_init();

    /* Auth */
    auth_init();

    /* Scheduler + Activity Manager */
    scheduler_init();
    activity_manager_init();

    /* Web server (depends on everything above) */
    web_view_manager_init();
    web_server_start();

    system_status_debug_print();
    ESP_LOGI(TAG, "Small OS initialization complete");
}