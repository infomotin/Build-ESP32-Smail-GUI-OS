#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// OS Components
#include "scheduler/task_scheduler.h"
#include "gpio/gpio_hal.h"
#include "interrupt/interrupt_handler.h"
#include "http/http_server.h"
#include "memory/memory_pool.h"
#include "network/wifi_manager.h"
#include "storage/nvs_storage.h"
#include "utils/logger.h"

#define TAG "ESP32_SMALL_OS"
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_NAME "ESP32 Small OS"

// GPIO pin configurations
static const gpio_pin_config_t gpio_configs[] = {
    // Button input (GPIO 0)
    {
        .pin = 0,
        .mode = GPIO_MODE_INPUT_PULLUP,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up = true,
        .pull_down = false
    },
    // Built-in LED output (GPIO 2)
    {
        .pin = 2,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up = false,
        .pull_down = false
    },
    // PWM output for dimming (GPIO 4)
    {
        .pin = 4,
        .mode = GPIO_MODE_PWM,
        .intr_type = GPIO_INTR_DISABLE,
        .config.pwm = {
            .frequency = 1000,
            .duty_cycle = 50,
            .resolution = 8
        }
    },
    // ADC input for sensor (GPIO 36)
    {
        .pin = 36,
        .mode = GPIO_MODE_ADC,
        .intr_type = GPIO_INTR_DISABLE,
        .config.adc = {
            .channel = ADC1_CHANNEL_0,
            .attenuation = ADC_ATTEN_DB_11,
            .bit_width = ADC_WIDTH_BIT_12
        }
    }
};

// HTTP handlers
static esp_err_t gpio_status_handler(const http_request_t *request, http_response_t *response);
static esp_err_t gpio_control_handler(const http_request_t *request, http_response_t *response);
static esp_err_t system_status_handler(const http_request_t *request, http_response_t *response);
static esp_err_t system_info_handler(const http_request_t *request, http_response_t *response);

// Button interrupt handler
static void button_interrupt_handler(uint8_t pin, void *arg)
{
    ESP_LOGI(TAG, "Button pressed on GPIO %d", pin);
    
    // Toggle built-in LED
    static bool led_state = false;
    led_state = !led_state;
    gpio_hal_set_level(2, led_state);
    
    // Update PWM duty cycle (cycle through 25%, 50%, 75%, 100%)
    static uint32_t pwm_duty = 25;
    pwm_duty = (pwm_duty % 100) + 25;
    gpio_hal_set_pwm_duty(4, pwm_duty);
    
    ESP_LOGI(TAG, "LED %s, PWM duty: %d%%", led_state ? "ON" : "OFF", pwm_duty);
}

// GPIO status handler
static esp_err_t gpio_status_handler(const http_request_t *request, http_response_t *response)
{
    cJSON *json = cJSON_CreateObject();
    cJSON *gpio_array = cJSON_CreateArray();
    
    for (int i = 0; i < sizeof(gpio_configs) / sizeof(gpio_configs[0]); i++) {
        cJSON *pin_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(pin_obj, "pin", gpio_configs[i].pin);
        
        if (gpio_configs[i].mode == GPIO_MODE_OUTPUT) {
            bool level = gpio_hal_get_level(gpio_configs[i].pin);
            cJSON_AddBoolToObject(pin_obj, "state", level);
        } else if (gpio_configs[i].mode == GPIO_MODE_ADC) {
            int adc_value = gpio_hal_read_adc(gpio_configs[i].pin);
            float voltage = 0.0;
            gpio_hal_read_adc_voltage(gpio_configs[i].pin, &voltage);
            cJSON_AddNumberToObject(pin_obj, "value", adc_value);
            cJSON_AddNumberToObject(pin_obj, "voltage", voltage);
        } else if (gpio_configs[i].mode == GPIO_MODE_PWM) {
            pwm_config_t pwm_config;
            gpio_hal_get_pwm_config(gpio_configs[i].pin, &pwm_config);
            cJSON_AddNumberToObject(pin_obj, "frequency", pwm_config.frequency);
            cJSON_AddNumberToObject(pin_obj, "duty_cycle", pwm_config.duty_cycle);
        }
        
        cJSON_AddItemToArray(gpio_array, pin_obj);
    }
    
    cJSON_AddItemToObject(json, "gpio", gpio_array);
    
    // Set response
    response->status = HTTP_STATUS_OK;
    response->body = cJSON_PrintUnformatted(json);
    response->body_length = strlen(response->body);
    strcpy(response->content_type_str, "application/json");
    
    cJSON_Delete(json);
    return ESP_OK;
}

// GPIO control handler
static esp_err_t gpio_control_handler(const http_request_t *request, http_response_t *response)
{
    // Parse JSON body
    cJSON *json = cJSON_Parse(request->body);
    if (!json) {
        response->status = HTTP_STATUS_BAD_REQUEST;
        response->body = strdup("{\"error\":\"Invalid JSON\"}");
        response->body_length = strlen(response->body);
        strcpy(response->content_type_str, "application/json");
        return ESP_OK;
    }
    
    cJSON *pin_obj = cJSON_GetObjectItem(json, "pin");
    cJSON *state_obj = cJSON_GetObjectItem(json, "state");
    cJSON *duty_obj = cJSON_GetObjectItem(json, "duty");
    cJSON *freq_obj = cJSON_GetObjectItem(json, "frequency");
    
    esp_err_t result = ESP_OK;
    
    if (pin_obj && state_obj) {
        int pin = cJSON_GetNumberValue(pin_obj);
        bool state = cJSON_IsTrue(state_obj);
        
        if (pin >= 0 && pin < 40) {
            gpio_hal_set_level(pin, state);
            ESP_LOGI(TAG, "Set GPIO %d to %s", pin, state ? "HIGH" : "LOW");
        } else {
            result = ESP_ERR_INVALID_ARG;
        }
    } else if (pin_obj && duty_obj) {
        int pin = cJSON_GetNumberValue(pin_obj);
        int duty = cJSON_GetNumberValue(duty_obj);
        
        if (pin >= 0 && pin < 40 && duty >= 0 && duty <= 100) {
            gpio_hal_set_pwm_duty(pin, duty);
            ESP_LOGI(TAG, "Set GPIO %d PWM duty to %d%%", pin, duty);
        } else {
            result = ESP_ERR_INVALID_ARG;
        }
    } else if (pin_obj && freq_obj) {
        int pin = cJSON_GetNumberValue(pin_obj);
        int freq = cJSON_GetNumberValue(freq_obj);
        
        if (pin >= 0 && pin < 40 && freq > 0 && freq <= 50000) {
            pwm_config_t pwm_config;
            gpio_hal_get_pwm_config(pin, &pwm_config);
            pwm_config.frequency = freq;
            gpio_hal_set_pwm(pin, &pwm_config);
            ESP_LOGI(TAG, "Set GPIO %d PWM frequency to %d Hz", pin, freq);
        } else {
            result = ESP_ERR_INVALID_ARG;
        }
    } else {
        result = ESP_ERR_INVALID_ARG;
    }
    
    // Set response
    if (result == ESP_OK) {
        response->status = HTTP_STATUS_OK;
        response->body = strdup("{\"status\":\"ok\"}");
    } else {
        response->status = HTTP_STATUS_BAD_REQUEST;
        response->body = strdup("{\"error\":\"Invalid parameters\"}");
    }
    
    response->body_length = strlen(response->body);
    strcpy(response->content_type_str, "application/json");
    
    cJSON_Delete(json);
    return ESP_OK;
}

// System status handler
static esp_err_t system_status_handler(const http_request_t *request, http_response_t *response)
{
    cJSON *json = cJSON_CreateObject();
    
    // System information
    cJSON_AddStringToObject(json, "system_name", SYSTEM_NAME);
    cJSON_AddStringToObject(json, "version", SYSTEM_VERSION);
    cJSON_AddNumberToObject(json, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(json, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(json, "min_free_heap", esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(json, "cpu_freq", esp_clk_cpu_freq() / 1000000);
    
    // Memory statistics
    memory_stats_t mem_stats;
    if (memory_pool_get_statistics(&mem_stats) == ESP_OK) {
        cJSON *mem_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(mem_obj, "total_memory", mem_stats.total_memory);
        cJSON_AddNumberToObject(mem_obj, "free_memory", mem_stats.free_memory);
        cJSON_AddNumberToObject(mem_obj, "allocated_memory", mem_stats.allocated_memory);
        cJSON_AddNumberToObject(mem_obj, "fragmentation_percent", mem_stats.fragmentation_percent);
        cJSON_AddItemToObject(json, "memory", mem_obj);
    }
    
    // Task scheduler statistics
    scheduler_stats_t sched_stats;
    if (scheduler_get_statistics(&sched_stats) == ESP_OK) {
        cJSON *sched_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(sched_obj, "total_tasks", sched_stats.total_tasks);
        cJSON_AddNumberToObject(sched_obj, "ready_tasks", sched_stats.ready_tasks);
        cJSON_AddNumberToObject(sched_obj, "blocked_tasks", sched_stats.blocked_tasks);
        cJSON_AddNumberToObject(sched_obj, "context_switches", sched_stats.context_switches);
        cJSON_AddItemToObject(json, "scheduler", sched_obj);
    }
    
    // GPIO statistics
    gpio_stats_t gpio_stats;
    if (gpio_hal_get_statistics(&gpio_stats) == ESP_OK) {
        cJSON *gpio_stats_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(gpio_stats_obj, "total_interrupts", gpio_stats.total_interrupts);
        cJSON_AddNumberToObject(gpio_stats_obj, "total_reads", gpio_stats.total_reads);
        cJSON_AddNumberToObject(gpio_stats_obj, "total_writes", gpio_stats.total_writes);
        cJSON_AddNumberToObject(gpio_stats_obj, "total_pwm_updates", gpio_stats.total_pwm_updates);
        cJSON_AddNumberToObject(gpio_stats_obj, "total_adc_reads", gpio_stats.total_adc_reads);
        cJSON_AddItemToObject(json, "gpio_stats", gpio_stats_obj);
    }
    
    // Set response
    response->status = HTTP_STATUS_OK;
    response->body = cJSON_PrintUnformatted(json);
    response->body_length = strlen(response->body);
    strcpy(response->content_type_str, "application/json");
    
    cJSON_Delete(json);
    return ESP_OK;
}

// System info handler
static esp_err_t system_info_handler(const http_request_t *request, http_response_t *response)
{
    const char *html_content = 
    "<!DOCTYPE html>"
    "<html><head><title>ESP32 Small OS</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>"
    "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;}"
    ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}"
    ".header{text-align:center;color:#333;margin-bottom:30px;}"
    ".status{display:flex;flex-wrap:wrap;gap:20px;margin:20px 0;}"
    ".status-item{flex:1;min-width:200px;padding:15px;background:#e8f4fd;border-radius:5px;text-align:center;}"
    ".gpio-controls{margin:20px 0;}"
    ".gpio-btn{padding:10px 20px;margin:5px;border:none;border-radius:5px;cursor:pointer;font-size:14px;}"
    ".gpio-btn.on{background:#4CAF50;color:white;}"
    ".gpio-btn.off{background:#f44336;color:white;}"
    ".slider{width:100%;margin:10px 0;}"
    ".refresh{background:#2196F3;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;}"
    "</style></head><body>"
    "<div class='container'>"
    "<div class='header'><h1>ESP32 Small OS</h1><p>Web-based GPIO Control Interface</p></div>"
    "<div class='status'>"
    "<div class='status-item'><h3>System Status</h3><p id='system-status'>Loading...</p></div>"
    "<div class='status-item'><h3>Memory Usage</h3><p id='memory-status'>Loading...</p></div>"
    "<div class='status-item'><h3>Uptime</h3><p id='uptime'>Loading...</p></div>"
    "</div>"
    "<div class='gpio-controls'>"
    "<h3>GPIO Controls</h3>"
    "<p><button class='gpio-btn on' onclick='setGPIO(2, true)'>LED ON</button>"
    "<button class='gpio-btn off' onclick='setGPIO(2, false)'>LED OFF</button></p>"
    "<p>PWM Duty: <input type='range' class='slider' id='pwm-slider' min='0' max='100' value='50' onchange='setPWM(4, this.value)'>"
    "<span id='pwm-value'>50%</span></p>"
    "<p><button class='refresh' onclick='refreshStatus()'>Refresh Status</button></p>"
    "</div>"
    "<script>"
    "function setGPIO(pin, state) {"
    "  fetch('/api/gpio/control', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({pin:pin, state:state})})"
    "  .then(r=>r.json()).then(d=>console.log(d));"
    "}"
    "function setPWM(pin, duty) {"
    "  document.getElementById('pwm-value').textContent = duty + '%';"
    "  fetch('/api/gpio/control', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({pin:pin, duty:duty})})"
    "  .then(r=>r.json()).then(d=>console.log(d));"
    "}"
    "function refreshStatus() {"
    "  fetch('/api/system/status').then(r=>r.json()).then(data=>{"
    "    document.getElementById('system-status').textContent = 'Free Heap: ' + Math.round(data.free_heap/1024) + 'KB';"
    "    document.getElementById('memory-status').textContent = 'Allocated: ' + Math.round(data.memory.allocated_memory/1024) + 'KB';"
    "    document.getElementById('uptime').textContent = Math.floor(data.uptime) + 's';"
    "  });"
    "}"
    "setInterval(refreshStatus, 5000);"
    "refreshStatus();"
    "</script></div></body></html>";
    
    response->status = HTTP_STATUS_OK;
    response->body = strdup(html_content);
    response->body_length = strlen(html_content);
    strcpy(response->content_type_str, "text/html");
    
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting %s v%s", SYSTEM_NAME, SYSTEM_VERSION);
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Initialize NVS storage wrapper
    ESP_ERROR_CHECK(nvs_storage_init());
    
    // Initialize memory pool
    memory_config_t mem_config = {
        .allocation_strategy = ALLOC_STRATEGY_FIRST_FIT,
        .enable_debugging = true,
        .enable_statistics = true,
        .enable_leak_detection = true,
        .enable_defragmentation = true,
        .defragmentation_threshold = 80,
        .leak_detection_interval = 60000
    };
    ESP_ERROR_CHECK(memory_pool_init(&mem_config));
    
    // Initialize GPIO
    ESP_ERROR_CHECK(gpio_hal_init());
    
    // Configure GPIO pins
    for (int i = 0; i < sizeof(gpio_configs) / sizeof(gpio_configs[0]); i++) {
        ESP_ERROR_CHECK(gpio_hal_configure_pin(&gpio_configs[i]));
        ESP_LOGI(TAG, "Configured GPIO %d as mode %d", gpio_configs[i].pin, gpio_configs[i].mode);
    }
    
    // Initialize interrupt system
    interrupt_config_t int_config = {
        .max_sources = 16,
        .max_events = 64,
        .debounce_time_ms = 50,
        .enable_statistics = true,
        .enable_priority_queue = true,
        .enable_debounce = true,
        .isr_stack_size = 2048,
        .task_priority = 5
    };
    ESP_ERROR_CHECK(interrupt_init(&int_config));
    
    // Register button interrupt
    ESP_ERROR_CHECK(interrupt_register_source(0, GPIO_INTR_NEGEDGE, button_interrupt_handler, NULL));
    
    // Initialize task scheduler
    scheduler_config_t sched_config = {
        .max_tasks = 16,
        .time_slice_ms = 10,
        .idle_timeout_ms = 100,
        .preemptive_enabled = false,
        .statistics_enabled = true
    };
    ESP_ERROR_CHECK(scheduler_init(&sched_config));
    
    // Initialize WiFi manager
    ESP_ERROR_CHECK(wifi_manager_init());
    
    // Initialize HTTP server
    http_server_config_t http_config = {
        .port = 80,
        .max_clients = 4,
        .request_timeout_ms = 5000,
        .keep_alive_timeout_ms = 30000,
        .max_request_size = 4096,
        .max_response_size = 8192,
        .enable_ssl = false,
        .enable_compression = true,
        .enable_caching = true,
        .enable_rate_limiting = true,
        .rate_limit_per_minute = 60,
        .enable_logging = true,
        .log_level = 1
    };
    ESP_ERROR_CHECK(http_server_init(&http_config));
    
    // Register HTTP endpoints
    http_endpoint_t gpio_status_endpoint = {
        .uri = "/api/gpio/status",
        .method = HTTP_METHOD_GET,
        .handler = gpio_status_handler,
        .requires_auth = false
    };
    ESP_ERROR_CHECK(http_server_register_endpoint(&gpio_status_endpoint));
    
    http_endpoint_t gpio_control_endpoint = {
        .uri = "/api/gpio/control",
        .method = HTTP_METHOD_POST,
        .handler = gpio_control_handler,
        .requires_auth = false
    };
    ESP_ERROR_CHECK(http_server_register_endpoint(&gpio_control_endpoint));
    
    http_endpoint_t system_status_endpoint = {
        .uri = "/api/system/status",
        .method = HTTP_METHOD_GET,
        .handler = system_status_handler,
        .requires_auth = false
    };
    ESP_ERROR_CHECK(http_server_register_endpoint(&system_status_endpoint));
    
    http_endpoint_t system_info_endpoint = {
        .uri = "/",
        .method = HTTP_METHOD_GET,
        .handler = system_info_handler,
        .requires_auth = false
    };
    ESP_ERROR_CHECK(http_server_register_endpoint(&system_info_endpoint));
    
    // Start HTTP server
    ESP_ERROR_CHECK(http_server_start());
    
    ESP_LOGI(TAG, "%s started successfully", SYSTEM_NAME);
    ESP_LOGI(TAG, "Web interface available at http://192.168.1.100");
    ESP_LOGI(TAG, "GPIO 0: Button (input with pull-up)");
    ESP_LOGI(TAG, "GPIO 2: LED (output)");
    ESP_LOGI(TAG, "GPIO 4: PWM output (1kHz, 50%% duty)");
    ESP_LOGI(TAG, "GPIO 36: ADC input (12-bit, 11dB attenuation)");
    
    // Main loop
    while (1) {
        interrupt_process_events();
        scheduler_run_once();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
