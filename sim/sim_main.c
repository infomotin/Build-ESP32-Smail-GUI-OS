/*
 * ESP32 Small OS Simulation Main
 * 
 * Simulation-compatible version of the main application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "sim_esp32.h"

// Simulation-specific includes for OS components
extern int scheduler_init(void);
extern int scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                                uint32_t stack_size, int priority);
extern void scheduler_run_once(void);
extern int scheduler_delay(uint32_t ms);

extern int gpio_hal_init(void);
extern int gpio_hal_set_level(uint8_t pin, bool level);
extern bool gpio_hal_get_level(uint8_t pin);

extern int nvs_flash_init(void);
extern int nvs_flash_erase(void);

extern int esp_wifi_init(void *config);
extern int esp_wifi_connect(void);
extern int esp_wifi_disconnect(void);
extern int esp_wifi_set_mode(int mode);
extern int esp_wifi_set_config(int interface, void *config);
extern int esp_wifi_start(void);
extern int esp_wifi_stop(void);

extern int esp_netif_init(void);
extern void* esp_netif_create_default_wifi_sta(void);
extern void* esp_netif_get_handle_from_ifkey(const char *key);

extern int esp_event_handler_register(void *base, int32_t id, void *handler, void *arg);

extern int http_server_init(void *config);
extern int http_server_start(void);

// Simulation constants
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_NAME "ESP32 Small OS (Simulation)"
#define SIMULATION_DURATION_MS 30000  // 30 seconds

// GPIO pin configurations
typedef struct {
    uint8_t pin;
    int mode;
    int intr_type;
    bool pull_up;
    bool pull_down;
} gpio_pin_config_t;

static const gpio_pin_config_t gpio_configs[] = {
    // Button input (GPIO 0)
    {
        .pin = 0,
        .mode = 1,  // INPUT_PULLUP
        .intr_type = 2,  // NEGEDGE
        .pull_up = true,
        .pull_down = false
    },
    // Built-in LED output (GPIO 2)
    {
        .pin = 2,
        .mode = 2,  // OUTPUT
        .intr_type = 0,  // DISABLE
        .pull_up = false,
        .pull_down = false
    },
    // PWM output for dimming (GPIO 4)
    {
        .pin = 4,
        .mode = 3,  // PWM
        .intr_type = 0,  // DISABLE
        .pull_up = false,
        .pull_down = false
    },
    // ADC input for sensor (GPIO 36)
    {
        .pin = 36,
        .mode = 4,  // ADC
        .intr_type = 0,  // DISABLE
        .pull_up = false,
        .pull_down = false
    }
};

// Simulation state
static bool simulation_running = true;
static uint32_t start_time = 0;

// Button interrupt handler simulation
static void button_interrupt_handler(uint8_t pin, void *arg)
{
    printf("[INTERRUPT] Button pressed on GPIO %d\n", pin);
    
    // Toggle built-in LED
    static bool led_state = false;
    led_state = !led_state;
    gpio_hal_set_level(2, led_state);
    
    // Simulate PWM duty cycle change
    static uint32_t pwm_duty = 25;
    pwm_duty = (pwm_duty % 100) + 25;
    printf("[GPIO] PWM duty cycle set to %d%%\n", pwm_duty);
    
    printf("[GPIO] LED %s, PWM duty: %d%%\n", led_state ? "ON" : "OFF", pwm_duty);
}

// System monitoring task
static void monitor_task(void *arg)
{
    static uint32_t counter = 0;
    counter++;
    
    printf("[MONITOR] System Status (tick %lu):\n", counter);
    printf("  Uptime: %lu ms\n", esp_timer_get_time() / 1000);
    printf("  Free Heap: %d bytes\n", esp_get_free_heap_size());
    printf("  CPU Freq: %lu Hz\n", esp_clk_cpu_freq());
    printf("  LED State: %s\n", gpio_hal_get_level(2) ? "ON" : "OFF");
    printf("  Button State: %s\n", gpio_hal_get_level(0) ? "PRESSED" : "RELEASED");
    
    // Simulate button press every 5 seconds
    if (counter % 50 == 0) {  // Every 5 seconds (50 * 100ms)
        printf("[SIM] Simulating button press...\n");
        button_interrupt_handler(0, NULL);
    }
    
    // Check if simulation should end
    if ((esp_timer_get_time() / 1000) - start_time >= SIMULATION_DURATION_MS) {
        printf("[SIM] Simulation duration reached, stopping...\n");
        simulation_running = false;
    }
    
    scheduler_delay(100);  // Run every 100ms
}

// LED blinking task
static void led_task(void *arg)
{
    static bool state = false;
    state = !state;
    gpio_hal_set_level(2, state);
    printf("[LED] LED toggled to %s\n", state ? "ON" : "OFF");
    scheduler_delay(1000);  // Toggle every 1 second
}

// WiFi simulation task
static void wifi_task(void *arg)
{
    static int wifi_state = 0;
    
    switch (wifi_state) {
        case 0:
            printf("[WIFI] Initializing WiFi...\n");
            esp_wifi_init(NULL);
            wifi_state = 1;
            break;
        case 1:
            printf("[WIFI] Starting WiFi...\n");
            esp_wifi_start();
            wifi_state = 2;
            break;
        case 2:
            printf("[WIFI] Connecting to WiFi...\n");
            esp_wifi_connect();
            wifi_state = 3;
            break;
        case 3:
            printf("[WIFI] WiFi connected (simulated)\n");
            wifi_state = 4;
            break;
        default:
            break;
    }
    
    scheduler_delay(2000);  // WiFi operations every 2 seconds
}

// HTTP server simulation task
static void http_task(void *arg)
{
    static int http_state = 0;
    
    switch (http_state) {
        case 0:
            printf("[HTTP] Initializing HTTP server...\n");
            http_server_init(NULL);
            http_state = 1;
            break;
        case 1:
            printf("[HTTP] Starting HTTP server...\n");
            http_server_start();
            printf("[HTTP] HTTP server started on port 80 (simulated)\n");
            http_state = 2;
            break;
        case 2:
            printf("[HTTP] Simulating web requests...\n");
            printf("[HTTP] GET /api/gpio/status -> {\"gpio\":[{\"pin\":2,\"state\":false}]}\n");
            printf("[HTTP] POST /api/gpio/control -> {\"status\":\"ok\"}\n");
            break;
        default:
            break;
    }
    
    scheduler_delay(3000);  // HTTP operations every 3 seconds
}

// GPIO simulation task
static void gpio_task(void *arg)
{
    // Simulate ADC reading
    static int adc_value = 2048;
    adc_value = (adc_value + 100) % 4096;
    float voltage = (adc_value / 4096.0f) * 3.3f;
    printf("[GPIO] ADC Read: %d (%.2fV)\n", adc_value, voltage);
    
    scheduler_delay(500);  // ADC reading every 500ms
}

// Initialize system components
static int initialize_system(void)
{
    printf("[INIT] Initializing %s v%s\n", SYSTEM_NAME, SYSTEM_VERSION);
    
    // Initialize NVS
    printf("[INIT] Initializing NVS...\n");
    if (nvs_flash_init() != ESP_OK) {
        printf("[INIT] Erasing NVS flash...\n");
        nvs_flash_erase();
        if (nvs_flash_init() != ESP_OK) {
            printf("[INIT] Failed to initialize NVS\n");
            return -1;
        }
    }
    
    // Initialize GPIO
    printf("[INIT] Initializing GPIO...\n");
    if (gpio_hal_init() != ESP_OK) {
        printf("[INIT] Failed to initialize GPIO\n");
        return -1;
    }
    
    // Configure GPIO pins
    for (int i = 0; i < sizeof(gpio_configs) / sizeof(gpio_configs[0]); i++) {
        printf("[INIT] Configured GPIO %d as mode %d\n", gpio_configs[i].pin, gpio_configs[i].mode);
    }
    
    // Initialize scheduler
    printf("[INIT] Initializing scheduler...\n");
    if (scheduler_init() != ESP_OK) {
        printf("[INIT] Failed to initialize scheduler\n");
        return -1;
    }
    
    // Initialize network
    printf("[INIT] Initializing network...\n");
    if (esp_netif_init() != ESP_OK) {
        printf("[INIT] Failed to initialize network\n");
        return -1;
    }
    
    esp_netif_create_default_wifi_sta();
    
    // Register event handlers
    esp_event_handler_register(NULL, 0, NULL, NULL);
    
    printf("[INIT] System initialized successfully\n");
    return 0;
}

// Create system tasks
static int create_tasks(void)
{
    printf("[TASKS] Creating system tasks...\n");
    
    // Create monitoring task
    if (scheduler_create_task("monitor", monitor_task, NULL, 4096, 2) != ESP_OK) {
        printf("[TASKS] Failed to create monitor task\n");
        return -1;
    }
    
    // Create LED task
    if (scheduler_create_task("led_blink", led_task, NULL, 2048, 1) != ESP_OK) {
        printf("[TASKS] Failed to create LED task\n");
        return -1;
    }
    
    // Create WiFi task
    if (scheduler_create_task("wifi_manager", wifi_task, NULL, 4096, 2) != ESP_OK) {
        printf("[TASKS] Failed to create WiFi task\n");
        return -1;
    }
    
    // Create HTTP task
    if (scheduler_create_task("http_server", http_task, NULL, 4096, 2) != ESP_OK) {
        printf("[TASKS] Failed to create HTTP task\n");
        return -1;
    }
    
    // Create GPIO task
    if (scheduler_create_task("gpio_monitor", gpio_task, NULL, 2048, 1) != ESP_OK) {
        printf("[TASKS] Failed to create GPIO task\n");
        return -1;
    }
    
    printf("[TASKS] All tasks created successfully\n");
    return 0;
}

// Main simulation application
void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  %s v%s\n", SYSTEM_NAME, SYSTEM_VERSION);
    printf("========================================\n");
    printf("\n");
    
    start_time = esp_timer_get_time() / 1000;
    
    // Initialize system
    if (initialize_system() != 0) {
        printf("[FATAL] System initialization failed\n");
        return;
    }
    
    // Create tasks
    if (create_tasks() != 0) {
        printf("[FATAL] Task creation failed\n");
        return;
    }
    
    printf("\n[SIM] Starting simulation loop...\n");
    printf("[SIM] Duration: %d seconds\n", SIMULATION_DURATION_MS / 1000);
    printf("[SIM] Press Ctrl+C to stop early\n\n");
    
    // Main simulation loop
    while (simulation_running) {
        scheduler_run_once();
        usleep(10000);  // 10ms delay
    }
    
    printf("\n[SIM] Simulation completed successfully\n");
    printf("[SIM] Total runtime: %lu ms\n", (esp_timer_get_time() / 1000) - start_time);
    printf("[SIM] Thank you for using ESP32 Small OS Simulation!\n");
}
