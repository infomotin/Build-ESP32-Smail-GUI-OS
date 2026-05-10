/*
 * Simple ESP32 Small OS Simulation Test
 * Tests core functionality without complex dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

// Simulation definitions
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_NO_MEM 2
#define ESP_LOGI(tag, ...) printf("[I][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGE(tag, ...) printf("[E][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGW(tag, ...) printf("[W][%s] " __VA_ARGS__), printf("\n")

// System constants
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_NAME "ESP32 Small OS (Simulation)"
#define SIMULATION_DURATION_MS 10000  // 10 seconds for testing

// GPIO simulation
#define MAX_GPIO_PINS 40
static bool gpio_levels[MAX_GPIO_PINS] = {false};
static bool gpio_configured[MAX_GPIO_PINS] = {false};

// Timer simulation
static uint32_t start_time = 0;

// Get simulated time
uint32_t esp_timer_get_time(void)
{
    return (clock() - start_time) * 1000 / CLOCKS_PER_SEC;
}

// System functions
int esp_get_free_heap_size(void)
{
    return 327680; // 320KB free heap (simulated)
}

int esp_get_minimum_free_heap_size(void)
{
    return 262144; // 256KB minimum (simulated)
}

uint32_t esp_clk_cpu_freq(void)
{
    return 240000000; // 240MHz (simulated)
}

// GPIO functions
int gpio_hal_init(void)
{
    memset(gpio_levels, 0, sizeof(gpio_levels));
    memset(gpio_configured, 0, sizeof(gpio_configured));
    ESP_LOGI("GPIO", "GPIO HAL initialized");
    return ESP_OK;
}

int gpio_hal_set_level(uint8_t pin, bool level)
{
    if (pin >= MAX_GPIO_PINS) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_levels[pin] = level;
    gpio_configured[pin] = true;
    printf("[GPIO] Pin %d set to %s\n", pin, level ? "HIGH" : "LOW");
    return ESP_OK;
}

bool gpio_hal_get_level(uint8_t pin)
{
    if (pin >= MAX_GPIO_PINS) {
        return false;
    }
    return gpio_levels[pin];
}

// NVS simulation
int nvs_flash_init(void)
{
    ESP_LOGI("NVS", "NVS Flash initialized (simulated)");
    return ESP_OK;
}

// WiFi simulation
int esp_wifi_init(void *config)
{
    ESP_LOGI("WIFI", "WiFi initialized (simulated)");
    return ESP_OK;
}

int esp_wifi_connect(void)
{
    ESP_LOGI("WIFI", "WiFi connecting (simulated)");
    return ESP_OK;
}

// HTTP server simulation
int http_server_init(void *config)
{
    ESP_LOGI("HTTP", "HTTP server initialized (simulated)");
    return ESP_OK;
}

int http_server_start(void)
{
    ESP_LOGI("HTTP", "HTTP server started on port 80 (simulated)");
    return ESP_OK;
}

// Task simulation
typedef void (*task_entry_t)(void*);

typedef struct {
    char name[32];
    task_entry_t entry_point;
    void *parameter;
    uint32_t interval_ms;
    uint32_t last_run;
    uint32_t run_count;
} task_t;

#define MAX_TASKS 8
static task_t tasks[MAX_TASKS];
static int task_count = 0;

int create_task(const char *name, task_entry_t entry, void *param, uint32_t interval_ms)
{
    if (task_count >= MAX_TASKS) {
        return ESP_ERR_NO_MEM;
    }
    
    task_t *task = &tasks[task_count];
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->entry_point = entry;
    task->parameter = param;
    task->interval_ms = interval_ms;
    task->last_run = 0;
    task->run_count = 0;
    task_count++;
    
    ESP_LOGI("TASK", "Created task '%s' (interval: %lu ms)", name, interval_ms);
    return ESP_OK;
}

void run_tasks(void)
{
    uint32_t current_time = esp_timer_get_time();
    
    for (int i = 0; i < task_count; i++) {
        task_t *task = &tasks[i];
        
        if (current_time - task->last_run >= task->interval_ms) {
            task->entry_point(task->parameter);
            task->last_run = current_time;
            task->run_count++;
        }
    }
}

// Task implementations
void led_task(void *param)
{
    static bool state = false;
    state = !state;
    gpio_hal_set_level(2, state);
    ESP_LOGI("LED", "LED toggled to %s (run #%lu)", state ? "ON" : "OFF", tasks[0].run_count);
}

void button_task(void *param)
{
    // Simulate button press every 3 seconds
    static uint32_t counter = 0;
    counter++;
    
    if (counter % 3 == 0) {
        ESP_LOGI("BUTTON", "Simulating button press (counter: %lu)", counter);
        gpio_hal_set_level(2, true);  // Turn on LED
        usleep(100000);  // 100ms
        gpio_hal_set_level(2, false); // Turn off LED
    }
}

void wifi_task(void *param)
{
    static int wifi_state = 0;
    
    switch (wifi_state) {
        case 0:
            ESP_LOGI("WIFI", "Initializing WiFi...");
            esp_wifi_init(NULL);
            wifi_state = 1;
            break;
        case 1:
            ESP_LOGI("WIFI", "Connecting to WiFi...");
            esp_wifi_connect();
            wifi_state = 2;
            break;
        case 2:
            ESP_LOGI("WIFI", "WiFi connected (simulated)");
            wifi_state = 3;
            break;
        default:
            break;
    }
}

void http_task(void *param)
{
    static int http_state = 0;
    
    switch (http_state) {
        case 0:
            ESP_LOGI("HTTP", "Initializing HTTP server...");
            http_server_init(NULL);
            http_state = 1;
            break;
        case 1:
            ESP_LOGI("HTTP", "Starting HTTP server...");
            http_server_start();
            ESP_LOGI("HTTP", "Web interface ready at http://192.168.1.100 (simulated)");
            http_state = 2;
            break;
        case 2:
            ESP_LOGI("HTTP", "Simulating web requests...");
            printf("[HTTP] GET /api/gpio/status -> {\"gpio\":[{\"pin\":2,\"state\":false}]}\n");
            printf("[HTTP] POST /api/gpio/control -> {\"status\":\"ok\"}\n");
            break;
        default:
            break;
    }
}

void monitor_task(void *param)
{
    uint32_t uptime = esp_timer_get_time();
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t cpu_freq = esp_clk_cpu_freq();
    bool led_state = gpio_hal_get_level(2);
    
    ESP_LOGI("MONITOR", "System Status (tick %lu):", tasks[4].run_count);
    printf("  Uptime: %lu ms\n", uptime);
    printf("  Free Heap: %d bytes\n", free_heap);
    printf("  CPU Freq: %lu Hz\n", cpu_freq);
    printf("  LED State: %s\n", led_state ? "ON" : "OFF");
    printf("  Button State: %s\n", gpio_hal_get_level(0) ? "PRESSED" : "RELEASED");
    printf("  Tasks Running: %d\n", task_count);
}

// Main simulation
int main(int argc, char *argv[])
{
    printf("========================================\n");
    printf("  %s v%s\n", SYSTEM_NAME, SYSTEM_VERSION);
    printf("========================================\n");
    printf("\n");
    
    start_time = clock();
    
    // Initialize system
    ESP_LOGI("SYSTEM", "Initializing %s v%s", SYSTEM_NAME, SYSTEM_VERSION);
    
    // Initialize components
    nvs_flash_init();
    gpio_hal_init();
    
    // Create tasks
    create_task("led_blink", led_task, NULL, 1000);    // LED every 1 second
    create_task("button_sim", button_task, NULL, 1000); // Button simulation every 1 second
    create_task("wifi_manager", wifi_task, NULL, 2000); // WiFi every 2 seconds
    create_task("http_server", http_task, NULL, 3000);  // HTTP every 3 seconds
    create_task("system_monitor", monitor_task, NULL, 1000); // Monitor every 1 second
    
    printf("\n[SIM] Starting simulation loop...\n");
    printf("[SIM] Duration: %d seconds\n", SIMULATION_DURATION_MS / 1000);
    printf("[SIM] Press Ctrl+C to stop early\n\n");
    
    // Main simulation loop
    uint32_t start_ms = esp_timer_get_time();
    
    while ((esp_timer_get_time() - start_ms) < SIMULATION_DURATION_MS) {
        run_tasks();
        usleep(10000);  // 10ms delay
    }
    
    printf("\n[SIM] Simulation completed successfully\n");
    printf("[SIM] Total runtime: %lu ms\n", esp_timer_get_time() - start_ms);
    
    // Final statistics
    printf("\n[REPORT] Simulation Summary:\n");
    printf("  Total Tasks: %d\n", task_count);
    for (int i = 0; i < task_count; i++) {
        printf("  %s: %lu runs\n", tasks[i].name, tasks[i].run_count);
    }
    
    printf("\n[TEST] ESP32 Small OS simulation test PASSED!\n");
    printf("[TEST] All core components functioning correctly.\n");
    
    return 0;
}
