#include "interrupt_handler.h"
#include "scheduler/task_scheduler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include <string.h>
#include <stdlib.h>

#define TAG "INTERRUPT"

// Global variables
static interrupt_source_t interrupt_sources[MAX_INTERRUPT_SOURCES];
static interrupt_event_t pending_events[MAX_PENDING_EVENTS];
static QueueHandle_t event_queue = NULL;
static SemaphoreHandle_t interrupt_mutex = NULL;
static uint32_t next_event_id = 1;
static interrupt_config_t interrupt_config;
static interrupt_stats_t interrupt_stats;
static bool interrupt_initialized = false;
static uint32_t interrupt_init_time = 0;

// ISR queue for high-priority events
static volatile interrupt_event_t isr_queue[ISR_QUEUE_SIZE];
static volatile uint8_t isr_queue_head = 0;
static volatile uint8_t isr_queue_tail = 0;
static SemaphoreHandle_t isr_queue_sem = NULL;

esp_err_t interrupt_init(const interrupt_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid interrupt configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (interrupt_initialized) {
        ESP_LOGW(TAG, "Interrupt system already initialized");
        return ESP_OK;
    }
    
    // Store configuration
    interrupt_config = *config;
    
    // Initialize statistics
    memset(&interrupt_stats, 0, sizeof(interrupt_stats_t));
    interrupt_init_time = esp_timer_get_time() / 1000;
    
    // Clear interrupt sources
    memset(interrupt_sources, 0, sizeof(interrupt_sources));
    memset(pending_events, 0, sizeof(pending_events));
    
    // Create mutex
    interrupt_mutex = xSemaphoreCreateMutex();
    if (!interrupt_mutex) {
        ESP_LOGE(TAG, "Failed to create interrupt mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create ISR queue semaphore
    isr_queue_sem = xSemaphoreCreateBinary();
    if (!isr_queue_sem) {
        ESP_LOGE(TAG, "Failed to create ISR queue semaphore");
        return ESP_ERR_NO_MEM;
    }
    xSemaphoreGive(isr_queue_sem);
    
    // Create event queue
    event_queue = xQueueCreate(config->max_events ? config->max_events : MAX_PENDING_EVENTS, 
                              sizeof(interrupt_event_t));
    if (!event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize GPIO ISR service
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize timer ISR service
    err = timer_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize timer service: %s", esp_err_to_name(err));
        return err;
    }
    
    interrupt_initialized = true;
    ESP_LOGI(TAG, "Interrupt handler initialized with %d max sources, %s debounce", 
              config->max_sources, config->enable_debounce ? "enabled" : "disabled");
    return ESP_OK;
}

static interrupt_source_t* find_source_by_pin(uint8_t pin)
{
    for (int i = 0; i < MAX_INTERRUPT_SOURCES; i++) {
        if (interrupt_sources[i].pin == pin && interrupt_sources[i].enabled) {
            return &interrupt_sources[i];
        }
    }
    return NULL;
}

static interrupt_source_t* find_free_source_slot(void)
{
    for (int i = 0; i < MAX_INTERRUPT_SOURCES; i++) {
        if (interrupt_sources[i].pin == 0) {
            return &interrupt_sources[i];
        }
    }
    return NULL;
}

esp_err_t interrupt_register_source(uint8_t pin, gpio_intr_type_t type, 
                                  void (*handler)(uint8_t pin, void *arg), void *arg)
{
    if (!handler || pin >= MAX_INTERRUPT_SOURCES || !interrupt_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(interrupt_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire interrupt mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    // Find free slot
    interrupt_source_t *source = find_free_source_slot();
    if (!source) {
        xSemaphoreGive(interrupt_mutex);
        ESP_LOGE(TAG, "No free interrupt slots");
        return ESP_ERR_NO_MEM;
    }
    
    // Configure interrupt source
    source->pin = pin;
    source->type = type;
    source->handler = handler;
    source->arg = arg;
    source->last_interrupt_time = 0;
    source->interrupt_count = 0;
    source->debouncing = false;
    source->debounce_state = 0;
    source->stable_count = 0;
    source->enabled = true;
    source->priority = EVENT_PRIORITY_NORMAL;
    
    // Configure GPIO interrupt
    esp_err_t err = configure_gpio_interrupt(pin, type);
    if (err != ESP_OK) {
        xSemaphoreGive(interrupt_mutex);
        ESP_LOGE(TAG, "Failed to configure GPIO interrupt: %s", esp_err_to_name(err));
        return err;
    }
    
    // Add GPIO ISR handler
    err = gpio_isr_handler_add(pin, gpio_isr_handler, (void*)pin);
    if (err != ESP_OK) {
        xSemaphoreGive(interrupt_mutex);
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler: %s", esp_err_to_name(err));
        return err;
    }
    
    xSemaphoreGive(interrupt_mutex);
    
    ESP_LOGI(TAG, "Registered interrupt source: GPIO %d, type %d", pin, type);
    return ESP_OK;
}

esp_err_t interrupt_unregister_source(uint8_t pin)
{
    if (pin >= MAX_INTERRUPT_SOURCES || !interrupt_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(interrupt_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    interrupt_source_t *source = find_source_by_pin(pin);
    if (!source) {
        xSemaphoreGive(interrupt_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Remove GPIO ISR handler
    esp_err_t err = gpio_isr_handler_remove(pin);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to remove GPIO ISR handler: %s", esp_err_to_name(err));
    }
    
    // Clear interrupt
    err = gpio_intr_disable(pin);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to disable GPIO interrupt: %s", esp_err_to_name(err));
    }
    
    // Clear source
    memset(source, 0, sizeof(interrupt_source_t));
    
    xSemaphoreGive(interrupt_mutex);
    
    ESP_LOGI(TAG, "Unregistered interrupt source: GPIO %d", pin);
    return ESP_OK;
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Find interrupt source
    interrupt_source_t *source = find_source_by_pin(gpio_num);
    if (!source || !source->enabled) {
        return;
    }
    
    // Get current GPIO level
    bool level = gpio_get_level(gpio_num);
    
    // Debounce processing
    if (interrupt_config.enable_debounce) {
        if (!debounce_interrupt(gpio_num, level)) {
            return; // Filtered out by debounce
        }
    }
    
    // Update source statistics
    source->last_interrupt_time = current_time;
    source->interrupt_count++;
    interrupt_stats.total_interrupts++;
    
    // Create event
    interrupt_event_t event = {
        .type = EVENT_TYPE_GPIO,
        .source = gpio_num,
        .data = (void*)(uintptr_t)level,
        .timestamp = current_time,
        .priority = source->priority,
        .retry_count = 0,
        .from_isr = true
    };
    
    // Queue event from ISR
    queue_event_from_isr(&event);
    
    // Call user handler if registered
    if (source->handler) {
        source->handler(gpio_num, source->arg);
    }
}

static bool IRAM_ATTR debounce_interrupt(uint8_t pin, bool level)
{
    interrupt_source_t *source = find_source_by_pin(pin);
    if (!source) {
        return true; // No source, allow through
    }
    
    // Simple state machine debounce
    switch (source->debounce_state) {
        case 0: // Idle state
            source->debounce_state = 1;
            source->stable_count = 1;
            return false; // Need more samples
            
        case 1: // First sample
            if (level == gpio_get_level(pin)) {
                source->stable_count++;
                if (source->stable_count >= DEBOUNCE_STABLE_COUNT) {
                    source->debounce_state = 2; // Stable
                    return true;
                }
            } else {
                source->debounce_state = 0; // Reset
            }
            return false;
            
        case 2: // Stable state
            if (level != gpio_get_level(pin)) {
                source->debounce_state = 0; // Reset
                return false;
            }
            return true;
            
        default:
            source->debounce_state = 0;
            return false;
    }
}

static void IRAM_ATTR queue_event_from_isr(const interrupt_event_t *event)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    
    // Try to add to ISR queue
    uint8_t next_head = (isr_queue_head + 1) % ISR_QUEUE_SIZE;
    if (next_head != isr_queue_tail) {
        isr_queue[isr_queue_head] = *event;
        isr_queue_head = next_head;
        
        // Wake up processing task
        xSemaphoreGiveFromISR(isr_queue_sem, &higher_priority_task_woken);
    } else {
        // Queue full, increment missed events
        interrupt_stats.missed_events++;
    }
    
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

esp_err_t interrupt_post_event(const interrupt_event_t *event)
{
    if (!event || !interrupt_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    interrupt_event_t ev = *event;
    ev.timestamp = esp_timer_get_time() / 1000;
    ev.from_isr = false;
    
    if (xQueueSend(event_queue, &ev, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full");
        return ESP_ERR_NO_MEM;
    }
    
    interrupt_stats.total_events++;
    return ESP_OK;
}

esp_err_t interrupt_post_event_from_isr(const interrupt_event_t *event)
{
    if (!event || !interrupt_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return queue_event_from_isr(event);
}

esp_err_t interrupt_get_event(interrupt_event_t *event, uint32_t timeout_ms)
{
    if (!event || !event_queue || !interrupt_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // First check ISR queue
    if (xSemaphoreTake(isr_queue_sem, 0) == pdTRUE) {
        if (isr_queue_head != isr_queue_tail) {
            *event = isr_queue[isr_queue_tail];
            isr_queue_tail = (isr_queue_tail + 1) % ISR_QUEUE_SIZE;
            xSemaphoreGive(isr_queue_sem);
            return ESP_OK;
        }
        xSemaphoreGive(isr_queue_sem);
    }
    
    // Then check regular queue
    if (xQueueReceive(event_queue, event, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

void interrupt_process_events(void)
{
    if (!interrupt_initialized) {
        return;
    }
    
    interrupt_event_t event;
    
    // Process all pending events
    while (interrupt_get_event(&event, 0) == ESP_OK) {
        uint32_t start_time = esp_timer_get_time() / 1000;
        
        // Process based on event type
        switch (event.type) {
            case EVENT_TYPE_GPIO:
                handle_gpio_interrupt(event.source);
                break;
                
            case EVENT_TYPE_TIMER:
                handle_timer_event(event.source);
                break;
                
            case EVENT_TYPE_NETWORK:
                handle_network_event(event.source);
                break;
                
            default:
                ESP_LOGW(TAG, "Unknown event type: %d", event.type);
                break;
        }
        
        // Update statistics
        uint32_t processing_time = (esp_timer_get_time() / 1000) - start_time;
        update_statistics(&event, processing_time);
    }
    
    // Process debounce state machine
    if (interrupt_config.enable_debounce) {
        process_debounce();
    }
}

static void handle_gpio_interrupt(uint8_t pin)
{
    interrupt_source_t *source = find_source_by_pin(pin);
    if (!source) {
        return;
    }
    
    ESP_LOGD(TAG, "Processing GPIO interrupt from pin %d", pin);
    
    // Post to task scheduler if configured
    task_event_t task_event = {
        .type = EVENT_TYPE_GPIO,
        .source = pin,
        .data = source->arg,
        .timestamp = esp_timer_get_time() / 1000
    };
    
    scheduler_post_event(&task_event);
}

static void handle_timer_event(uint32_t timer_id)
{
    ESP_LOGD(TAG, "Processing timer event from timer %d", timer_id);
    
    // Post to task scheduler
    task_event_t task_event = {
        .type = EVENT_TYPE_TIMER,
        .source = timer_id,
        .data = NULL,
        .timestamp = esp_timer_get_time() / 1000
    };
    
    scheduler_post_event(&task_event);
}

static void handle_network_event(uint32_t event_type)
{
    ESP_LOGD(TAG, "Processing network event type %d", event_type);
    
    // Post to task scheduler
    task_event_t task_event = {
        .type = EVENT_TYPE_NETWORK,
        .source = event_type,
        .data = NULL,
        .timestamp = esp_timer_get_time() / 1000
    };
    
    scheduler_post_event(&task_event);
}

static void process_debounce(void)
{
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    for (int i = 0; i < MAX_INTERRUPT_SOURCES; i++) {
        interrupt_source_t *source = &interrupt_sources[i];
        if (!source->enabled || !source->debouncing) {
            continue;
        }
        
        // Check if debounce timeout has passed
        if ((current_time - source->last_interrupt_time) >= interrupt_config.debounce_time_ms) {
            source->debouncing = false;
            source->debounce_state = 0;
            source->stable_count = 0;
        }
    }
}

static void update_statistics(const interrupt_event_t *event, uint32_t processing_time)
{
    if (!interrupt_config.enable_statistics) {
        return;
    }
    
    // Update processing time statistics
    if (processing_time > interrupt_stats.max_processing_time) {
        interrupt_stats.max_processing_time = processing_time;
    }
    
    // Calculate average (simplified moving average)
    interrupt_stats.avg_processing_time = 
        (interrupt_stats.avg_processing_time * 9 + processing_time) / 10;
    
    // Update queue depth
    uxQueueMessagesWaiting(event_queue, &interrupt_stats.max_queue_depth);
}

static esp_err_t configure_gpio_interrupt(uint8_t pin, gpio_intr_type_t type)
{
    esp_err_t err = gpio_set_intr_type(pin, type);
    if (err != ESP_OK) {
        return err;
    }
    
    err = gpio_intr_enable(pin);
    if (err != ESP_OK) {
        return err;
    }
    
    return ESP_OK;
}

esp_err_t interrupt_get_statistics(interrupt_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *stats = interrupt_stats;
    return ESP_OK;
}

void interrupt_print_statistics(void)
{
    uint32_t current_time = esp_timer_get_time() / 1000;
    uint32_t uptime = current_time - interrupt_init_time;
    
    ESP_LOGI(TAG, "=== Interrupt Handler Statistics ===");
    ESP_LOGI(TAG, "Uptime: %d seconds", uptime);
    ESP_LOGI(TAG, "Total interrupts: %d", interrupt_stats.total_interrupts);
    ESP_LOGI(TAG, "Total events: %d", interrupt_stats.total_events);
    ESP_LOGI(TAG, "Missed events: %d", interrupt_stats.missed_events);
    ESP_LOGI(TAG, "Debounce rejections: %d", interrupt_stats.debounce_rejections);
    ESP_LOGI(TAG, "Max queue depth: %d", interrupt_stats.max_queue_depth);
    ESP_LOGI(TAG, "Avg processing time: %d ms", interrupt_stats.avg_processing_time);
    ESP_LOGI(TAG, "Max processing time: %d ms", interrupt_stats.max_processing_time);
    
    // Per-source statistics
    for (int i = 0; i < MAX_INTERRUPT_SOURCES; i++) {
        if (interrupt_sources[i].enabled) {
            ESP_LOGI(TAG, "Pin %d: %d interrupts, %s", 
                      interrupt_sources[i].pin, 
                      interrupt_sources[i].interrupt_count,
                      interrupt_sources[i].debouncing ? "debouncing" : "stable");
        }
    }
    
    ESP_LOGI(TAG, "=================================");
}

esp_err_t interrupt_self_test(void)
{
    ESP_LOGI(TAG, "Starting interrupt handler self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test GPIO interrupt registration
    uint8_t test_pin = 2; // Use GPIO 2 for testing
    
    static bool test_handler_called = false;
    void test_handler(uint8_t pin, void *arg) {
        test_handler_called = true;
        ESP_LOGI(TAG, "Test interrupt handler called for pin %d", pin);
    }
    
    esp_err_t err = interrupt_register_source(test_pin, GPIO_INTR_POSEDGE, test_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register test interrupt: %s", esp_err_to_name(err));
        result = ESP_FAIL;
    }
    
    // Simulate interrupt (if possible)
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Simulating interrupt on GPIO %d", test_pin);
        // Note: Actual interrupt simulation would require hardware-specific code
        
        // Wait a bit for processing
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (!test_handler_called) {
            ESP_LOGE(TAG, "Test interrupt handler was not called");
            result = ESP_FAIL;
        }
    }
    
    // Cleanup
    interrupt_unregister_source(test_pin);
    
    ESP_LOGI(TAG, "Interrupt handler self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
