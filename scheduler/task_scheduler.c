#include "task_scheduler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>

#define TAG "SCHEDULER"
#define MAX_TASKS 16
#define IDLE_STACK_SIZE 2048
#define TIME_SLICE_MS 10
#define SCHEDULER_TIMER_PERIOD_MS 1

// Global variables
static task_cb_t task_pool[MAX_TASKS];
static task_cb_t *ready_queue[TASK_PRIORITY_COUNT] = {NULL};
static task_cb_t *current_task = NULL;
static QueueHandle_t event_queue = NULL;
static SemaphoreHandle_t scheduler_mutex = NULL;
static esp_timer_handle_t scheduler_timer = NULL;
static uint32_t next_task_id = 1;
static uint32_t scheduler_time = 0;
static uint32_t scheduler_tick = 0;
static scheduler_config_t scheduler_config;
static scheduler_stats_t scheduler_stats;
static bool scheduler_running = false;

// Context saving for ESP32
typedef struct {
    uint32_t pc;
    uint32_t sp;
    uint32_t a0, a1, a2, a3, a4, a5;
    uint32_t a6, a7, a8, a9, a10, a11;
    uint32_t a12, a13, a14, a15;
    uint32_t ps;
} cpu_context_t;

esp_err_t scheduler_init(const scheduler_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Invalid scheduler configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Store configuration
    scheduler_config = *config;
    
    // Initialize task pool
    memset(task_pool, 0, sizeof(task_pool));
    memset(ready_queue, 0, sizeof(ready_queue));
    
    // Initialize statistics
    memset(&scheduler_stats, 0, sizeof(scheduler_stats_t));
    
    // Create mutex
    scheduler_mutex = xSemaphoreCreateMutex();
    if (!scheduler_mutex) {
        ESP_LOGE(TAG, "Failed to create scheduler mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Create event queue
    event_queue = xQueueCreate(config->max_events ? config->max_events : EVENT_QUEUE_SIZE, 
                              sizeof(task_event_t));
    if (!event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create scheduler timer
    esp_timer_create_args_t timer_args = {
        .callback = scheduler_timer_callback,
        .name = "scheduler_timer"
    };
    
    esp_err_t err = esp_timer_create(&timer_args, &scheduler_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create scheduler timer: %s", esp_err_to_name(err));
        return err;
    }
    
    // Start timer
    err = esp_timer_start_periodic(scheduler_timer, SCHEDULER_TIMER_PERIOD_MS * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start scheduler timer: %s", esp_err_to_name(err));
        return err;
    }
    
    // Create idle task
    uint32_t idle_task_id;
    err = scheduler_create_task("idle", idle_task_entry, NULL, 
                             IDLE_STACK_SIZE, TASK_PRIORITY_IDLE, &idle_task_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create idle task: %s", esp_err_to_name(err));
        return err;
    }
    
    scheduler_running = true;
    ESP_LOGI(TAG, "Scheduler initialized with %d max tasks, %s", 
              config->max_tasks, config->preemptive_enabled ? "preemptive" : "cooperative");
    return ESP_OK;
}

static void add_to_ready_queue(task_cb_t *task)
{
    if (!task) return;
    
    task->state = TASK_STATE_READY;
    
    // Insert at head of priority queue
    task->next = ready_queue[task->priority];
    task->prev = NULL;
    
    if (ready_queue[task->priority]) {
        ready_queue[task->priority]->prev = task;
    }
    
    ready_queue[task->priority] = task;
    scheduler_stats.ready_tasks++;
}

static task_cb_t* get_next_ready_task(void)
{
    // Find highest priority ready task
    for (int priority = TASK_PRIORITY_CRITICAL; priority >= TASK_PRIORITY_IDLE; priority--) {
        if (ready_queue[priority]) {
            task_cb_t *task = ready_queue[priority];
            
            // Remove from queue
            ready_queue[priority] = task->next;
            if (task->next) {
                task->next->prev = NULL;
            }
            
            task->next = NULL;
            task->prev = NULL;
            
            scheduler_stats.ready_tasks--;
            return task;
        }
    }
    return NULL;
}

static void remove_from_ready_queue(task_cb_t *task)
{
    if (!task || !task->prev && ready_queue[task->priority] != task) {
        return; // Task not in queue
    }
    
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        ready_queue[task->priority] = task->next;
    }
    
    if (task->next) {
        task->next->prev = task->prev;
    }
    
    task->next = NULL;
    task->prev = NULL;
}

esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                             uint32_t stack_size, task_priority_t priority, uint32_t *task_id)
{
    if (!name || !entry || priority >= TASK_PRIORITY_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(scheduler_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire scheduler mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    // Find free task control block
    task_cb_t *task = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].id == 0) {
            task = &task_pool[i];
            break;
        }
    }
    
    if (!task) {
        xSemaphoreGive(scheduler_mutex);
        ESP_LOGE(TAG, "No free task slots");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize task control block
    task->id = next_task_id++;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->entry_point = entry;
    task->parameter = param;
    task->stack_size = stack_size;
    task->wake_time = 0;
    task->timeout = 0;
    task->run_count = 0;
    task->total_run_time = 0;
    task->last_run_time = 0;
    task->next = NULL;
    task->prev = NULL;
    
    // Allocate stack
    task->stack_ptr = (uint32_t*)malloc(stack_size);
    if (!task->stack_ptr) {
        xSemaphoreGive(scheduler_mutex);
        ESP_LOGE(TAG, "Failed to allocate stack for task %s", name);
        return ESP_ERR_NO_MEM;
    }
    
    task->stack_top = (uint32_t)task->stack_ptr + stack_size;
    
    // Initialize stack frame
    cpu_context_t *ctx = (cpu_context_t*)(task->stack_top - sizeof(cpu_context_t));
    memset(ctx, 0, sizeof(cpu_context_t));
    ctx->pc = (uint32_t)entry;
    ctx->sp = (uint32_t)ctx;
    ctx->ps = 0x40020; // Initial processor status
    
    // Add to ready queue
    add_to_ready_queue(task);
    
    // Update statistics
    scheduler_stats.total_tasks++;
    
    if (task_id) {
        *task_id = task->id;
    }
    
    xSemaphoreGive(scheduler_mutex);
    
    ESP_LOGI(TAG, "Created task %s (ID: %d, Priority: %d, Stack: %d)", 
              name, task->id, priority, stack_size);
    return ESP_OK;
}

esp_err_t scheduler_delete_task(uint32_t task_id)
{
    if (xSemaphoreTake(scheduler_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Find task
    task_cb_t *task = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].id == task_id) {
            task = &task_pool[i];
            break;
        }
    }
    
    if (!task) {
        xSemaphoreGive(scheduler_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Remove from ready queue if present
    if (task->state == TASK_STATE_READY) {
        remove_from_ready_queue(task);
    }
    
    // Free stack
    if (task->stack_ptr) {
        free(task->stack_ptr);
        task->stack_ptr = NULL;
    }
    
    // Mark as deleted
    task->state = TASK_STATE_DELETED;
    task->id = 0;
    
    // Update statistics
    scheduler_stats.total_tasks--;
    
    xSemaphoreGive(scheduler_mutex);
    
    ESP_LOGI(TAG, "Deleted task ID %d", task_id);
    return ESP_OK;
}

void scheduler_run_once(void)
{
    if (!scheduler_running) return;
    
    // Update time
    scheduler_time = esp_timer_get_time() / 1000; // Convert to milliseconds
    scheduler_tick++;
    
    // Process timeouts
    process_timeouts();
    
    // Get next task to run
    task_cb_t *next_task = get_next_ready_task();
    if (!next_task) {
        // No ready tasks, run idle
        update_statistics();
        return;
    }
    
    // Context switch if needed
    if (current_task != next_task) {
        ESP_LOGD(TAG, "Switching from %s to %s", 
                  current_task ? current_task->name : "none", next_task->name);
        
        context_switch(current_task, next_task);
        current_task = next_task;
    }
    
    // Execute task
    uint32_t start_time = scheduler_time;
    current_task->state = TASK_STATE_RUNNING;
    current_task->last_run_time = start_time;
    current_task->run_count++;
    
    // Execute task entry point
    current_task->entry_point(current_task->parameter);
    
    // Update statistics
    uint32_t run_time = scheduler_time - start_time;
    current_task->total_run_time += run_time;
    update_statistics();
    
    // Handle time slice if preemptive
    if (scheduler_config.preemptive_enabled && run_time >= scheduler_config.time_slice_ms) {
        current_task->state = TASK_STATE_READY;
        add_to_ready_queue(current_task);
        current_task = NULL;
    }
}

void scheduler_run(void)
{
    ESP_LOGI(TAG, "Scheduler started");
    
    while (scheduler_running) {
        scheduler_run_once();
        
        // Yield to other tasks
        if (current_task && current_task->priority != TASK_PRIORITY_IDLE) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    ESP_LOGI(TAG, "Scheduler stopped");
}

esp_err_t scheduler_yield(void)
{
    if (!current_task || !scheduler_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(scheduler_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    current_task->state = TASK_STATE_READY;
    add_to_ready_queue(current_task);
    current_task = NULL;
    
    xSemaphoreGive(scheduler_mutex);
    
    return ESP_OK;
}

esp_err_t scheduler_delay(uint32_t ms)
{
    if (!current_task || !scheduler_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(scheduler_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    current_task->state = TASK_STATE_BLOCKED;
    current_task->timeout = scheduler_time + ms;
    current_task->wake_time = scheduler_time + ms;
    
    scheduler_stats.blocked_tasks++;
    
    xSemaphoreGive(scheduler_mutex);
    
    // Force context switch
    scheduler_yield();
    
    return ESP_OK;
}

esp_err_t scheduler_post_event(const task_event_t *event)
{
    if (!event || !event_queue) {
        return ESP_ERR_INVALID_ARG;
    }
    
    task_event_t ev = *event;
    ev.timestamp = scheduler_time;
    
    if (xQueueSend(event_queue, &ev, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue full");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

esp_err_t scheduler_get_event(task_event_t *event, uint32_t timeout_ms)
{
    if (!event || !event_queue) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xQueueReceive(event_queue, event, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

static void idle_task_entry(void *param)
{
    ESP_LOGI(TAG, "Idle task started");
    
    while (scheduler_running) {
        // Power management in idle
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
        
        // Update idle time statistics
        scheduler_stats.idle_time += SCHEDULER_TIMER_PERIOD_MS;
        
        vTaskDelay(pdMS_TO_TICKS(SCHEDULER_TIMER_PERIOD_MS));
    }
    
    ESP_LOGI(TAG, "Idle task ended");
    vTaskDelete(NULL);
}

static void scheduler_timer_callback(void *arg)
{
    // This timer callback runs in ISR context
    // Just increment tick counter
    scheduler_tick++;
}

static void process_timeouts(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        task_cb_t *task = &task_pool[i];
        if (task->id != 0 && task->state == TASK_STATE_BLOCKED && 
            task->timeout > 0 && scheduler_time >= task->timeout) {
            
            task->state = TASK_STATE_READY;
            add_to_ready_queue(task);
            scheduler_stats.blocked_tasks--;
            
            ESP_LOGD(TAG, "Task %s timeout reached", task->name);
        }
    }
}

static void context_switch(task_cb_t *from_task, task_cb_t *to_task)
{
    if (from_task) {
        // Save context (simplified)
        from_task->state = TASK_STATE_READY;
        scheduler_stats.context_switches++;
    }
    
    if (to_task) {
        // Load context (simplified)
        to_task->state = TASK_STATE_RUNNING;
    }
}

static void update_statistics(void)
{
    if (!scheduler_config.statistics_enabled) return;
    
    scheduler_stats.total_run_time += SCHEDULER_TIMER_PERIOD_MS;
    
    // Calculate average response time
    if (scheduler_stats.context_switches > 0) {
        scheduler_stats.avg_response_time = scheduler_stats.total_run_time / scheduler_stats.context_switches;
    }
}

esp_err_t scheduler_get_statistics(scheduler_stats_t *stats)
{
    if (!stats) return ESP_ERR_INVALID_ARG;
    
    *stats = scheduler_stats;
    return ESP_OK;
}

void scheduler_print_statistics(void)
{
    ESP_LOGI(TAG, "=== Scheduler Statistics ===");
    ESP_LOGI(TAG, "Total tasks: %d", scheduler_stats.total_tasks);
    ESP_LOGI(TAG, "Ready tasks: %d", scheduler_stats.ready_tasks);
    ESP_LOGI(TAG, "Blocked tasks: %d", scheduler_stats.blocked_tasks);
    ESP_LOGI(TAG, "Context switches: %d", scheduler_stats.context_switches);
    ESP_LOGI(TAG, "Idle time: %d ms", scheduler_stats.idle_time);
    ESP_LOGI(TAG, "Total run time: %d ms", scheduler_stats.total_run_time);
    ESP_LOGI(TAG, "Avg response time: %d ms", scheduler_stats.avg_response_time);
    ESP_LOGI(TAG, "Max response time: %d ms", scheduler_stats.max_response_time);
    ESP_LOGI(TAG, "Current task: %s", current_task ? current_task->name : "none");
    ESP_LOGI(TAG, "Scheduler time: %d ms", scheduler_time);
    ESP_LOGI(TAG, "Scheduler tick: %d", scheduler_tick);
    ESP_LOGI(TAG, "===========================");
}

uint32_t scheduler_get_time(void)
{
    return scheduler_time;
}

uint32_t scheduler_get_tick_count(void)
{
    return scheduler_tick;
}

task_cb_t* scheduler_get_current_task(void)
{
    return current_task;
}

esp_err_t scheduler_get_task_by_id(uint32_t task_id, task_cb_t **task)
{
    if (!task) return ESP_ERR_INVALID_ARG;
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].id == task_id) {
            *task = &task_pool[i];
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

void scheduler_dump_tasks(void)
{
    ESP_LOGI(TAG, "=== Task Dump ===");
    for (int i = 0; i < MAX_TASKS; i++) {
        task_cb_t *task = &task_pool[i];
        if (task->id != 0) {
            ESP_LOGI(TAG, "Task %d: %s, State: %d, Priority: %d, Runs: %d", 
                      task->id, task->name, task->state, task->priority, task->run_count);
        }
    }
    ESP_LOGI(TAG, "================");
}
