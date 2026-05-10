/*
 * ESP32 Small OS Simulation Runner
 * 
 * Simulates the Small OS behavior on a PC for testing and debugging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include "sim_esp32.h"

#ifdef SIMULATION

/* Simulation definitions */
#define ESP_OK 0
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_NO_MEM 2
#define ESP_ERR_TIMEOUT 3
#define ESP_LOGI(tag, ...) printf("[I][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGE(tag, ...) printf("[E][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGW(tag, ...) printf("[W][%s] " __VA_ARGS__), printf("\n")
#define ESP_LOGD(tag, ...) printf("[D][%s] " __VA_ARGS__), printf("\n")

/* Task scheduler types */
typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_DELETED
} task_state_t;

typedef enum {
    TASK_PRIORITY_IDLE = 0,
    TASK_PRIORITY_LOW = 1,
    TASK_PRIORITY_NORMAL = 2,
    TASK_PRIORITY_HIGH = 3,
    TASK_PRIORITY_CRITICAL = 4,
    TASK_PRIORITY_COUNT = 5
} task_priority_t;

typedef void (*task_entry_t)(void*);

typedef struct task_cb {
    uint32_t id;
    char name[32];
    task_state_t state;
    task_priority_t priority;
    task_entry_t entry_point;
    void *parameter;
    uint32_t stack_size;
    uint32_t *stack_ptr;
    uint32_t stack_top;
    uint32_t wake_time;
    uint32_t timeout;
    uint32_t run_count;
    uint32_t total_run_time;
    uint32_t last_run_time;
    struct task_cb *next;
    struct task_cb *prev;
} task_cb_t;

/* Scheduler simulator state */
#define MAX_TASKS 16
static task_cb_t task_pool[MAX_TASKS];
static task_cb_t *ready_queue[TASK_PRIORITY_COUNT] = {NULL};
static task_cb_t *current_task = NULL;
static uint32_t next_task_id = 1;
static uint32_t scheduler_time = 0;
static uint32_t scheduler_tick = 0;
static bool scheduler_running = false;

/* Simulated GPIO state */
#define MAX_GPIO_PINS 40
static bool gpio_levels[MAX_GPIO_PINS] = {false};
static bool gpio_configured[MAX_GPIO_PINS] = {false};

/* Simulated timer */
static struct timeval timer_start;

/* Timer callback simulation */
static void timer_callback(void)
{
    scheduler_tick++;
    scheduler_time++;
}

/* Get simulated time */
uint32_t esp_timer_get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec - timer_start.tv_sec) * 1000 + 
           (tv.tv_usec - timer_start.tv_usec) / 1000;
}

/* Initialize scheduler */
void scheduler_init(void)
{
    memset(task_pool, 0, sizeof(task_pool));
    memset(ready_queue, 0, sizeof(ready_queue));
    gettimeofday(&timer_start, NULL);
    scheduler_running = true;
    ESP_LOGI("SIM", "Scheduler initialized");
}

/* Schedule check triggers */
static void schedule_check_triggers(void)
{
    /* Check scheduled activities - placeholder for future implementation */
}

/* Timer callback wrapper */
static void scheduler_timer_callback(void *arg)
{
    timer_callback();
}

/* Create a task */
int scheduler_create_task(const char *name, void (*entry)(void*), void *param,
                          uint32_t stack_size, int priority)
{
    if (!name || !entry || priority >= TASK_PRIORITY_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    task_cb_t *task = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].id == 0) {
            task = &task_pool[i];
            break;
        }
    }
    
    if (!task) {
        return ESP_ERR_NO_MEM;
    }
    
    task->id = next_task_id++;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->entry_point = entry;
    task->parameter = param;
    task->stack_size = stack_size;
    task->run_count = 0;
    task->next = ready_queue[priority];
    if (ready_queue[priority]) {
        ready_queue[priority]->prev = task;
    }
    ready_queue[priority] = task;
    
    ESP_LOGI("SIM", "Created task %s (ID: %d, Priority: %d)", name, task->id, priority);
    return ESP_OK;
}

/* Run one scheduler iteration */
void scheduler_run_once(void)
{
    schedule_check_triggers();
    
    scheduler_time = esp_timer_get_time() / 1000;
    scheduler_tick++;
    
    /* Process timeouts */
    for (int i = 0; i < MAX_TASKS; i++) {
        task_cb_t *task = &task_pool[i];
        if (task->id != 0 && task->state == TASK_STATE_BLOCKED &&
            task->timeout > 0 && scheduler_time >= task->timeout) {
            task->state = TASK_STATE_READY;
            task->next = ready_queue[task->priority];
            if (task->next) {
                task->next->prev = task;
            }
            ready_queue[task->priority] = task;
            ESP_LOGI("SIM", "Task %s timeout reached", task->name);
        }
    }
    
    /* Find highest priority ready task */
    for (int priority = TASK_PRIORITY_CRITICAL; priority >= TASK_PRIORITY_IDLE; priority--) {
        if (ready_queue[priority]) {
            task_cb_t *task = ready_queue[priority];
            ready_queue[priority] = task->next;
            if (task->next) {
                task->next->prev = NULL;
            }
            task->next = NULL;
            task->prev = NULL;
            current_task = task;
            break;
        }
    }
    
    /* Execute task */
    if (current_task) {
        current_task->state = TASK_STATE_RUNNING;
        ESP_LOGD("SIM", "Running task %s", current_task->name);
        current_task->entry_point(current_task->parameter);
        current_task->state = TASK_STATE_READY;
        current_task->run_count++;
        current_task->next = ready_queue[current_task->priority];
        if (current_task->next) {
            current_task->next->prev = current_task;
        }
        ready_queue[current_task->priority] = current_task;
    }
}

/* Run scheduler loop */
void scheduler_run(void)
{
    ESP_LOGI("SIM", "Scheduler started");
    
    while (scheduler_running) {
        scheduler_run_once();
        usleep(10000);  /* 10ms delay */
    }
    
    ESP_LOGI("SIM", "Scheduler stopped");
}

/* Stop scheduler */
void scheduler_stop(void)
{
    scheduler_running = false;
}

/* Delay task */
int scheduler_delay(uint32_t ms)
{
    if (!current_task || !scheduler_running) {
        return ESP_ERR_INVALID_ARG;
    }
    
    current_task->state = TASK_STATE_BLOCKED;
    current_task->timeout = scheduler_time + ms;
    
    /* Remove from ready queue */
    if (current_task->prev) {
        current_task->prev->next = current_task->next;
    } else {
        ready_queue[current_task->priority] = current_task->next;
    }
    if (current_task->next) {
        current_task->next->prev = current_task->prev;
    }
    current_task->next = NULL;
    current_task->prev = NULL;
    
    return ESP_OK;
}

/* Yield to next task */
int scheduler_yield(void)
{
    if (!current_task || !scheduler_running) {
        return ESP_ERR_INVALID_ARG;
    }
    
    current_task->state = TASK_STATE_READY;
    current_task->next = ready_queue[current_task->priority];
    if (current_task->next) {
        current_task->next->prev = current_task;
    }
    ready_queue[current_task->priority] = current_task;
    current_task = NULL;
    
    return ESP_OK;
}

/* Simulated GPIO functions */
int gpio_hal_init(void)
{
    memset(gpio_levels, 0, sizeof(gpio_levels));
    memset(gpio_configured, 0, sizeof(gpio_configured));
    ESP_LOGI("SIM", "GPIO HAL initialized");
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

/* Simulated memory tracking */
static void* sim_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr) {
        printf("[MEM] Allocated %zu bytes at %p\n", size, ptr);
    } else {
        printf("[MEM] Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

static void sim_free(void *ptr)
{
    if (ptr) {
        printf("[MEM] Freed memory at %p\n", ptr);
        free(ptr);
    }
}

/* Simulated NVS functions */
int nvs_flash_init(void)
{
    ESP_LOGI("SIM", "NVS Flash initialized (simulated)");
    return ESP_OK;
}

int nvs_flash_erase(void)
{
    ESP_LOGI("SIM", "NVS Flash erased (simulated)");
    return ESP_OK;
}

/* Simulated WiFi functions */
int esp_wifi_init(void *config)
{
    ESP_LOGI("SIM", "WiFi initialized (simulated)");
    return ESP_OK;
}

int esp_wifi_connect(void)
{
    ESP_LOGI("SIM", "WiFi connecting (simulated)");
    return ESP_OK;
}

int esp_wifi_disconnect(void)
{
    ESP_LOGI("SIM", "WiFi disconnected (simulated)");
    return ESP_OK;
}

/* Simulated HTTP server functions */
int http_server_init(void *config)
{
    ESP_LOGI("SIM", "HTTP server initialized (simulated)");
    return ESP_OK;
}

int http_server_start(void)
{
    ESP_LOGI("SIM", "HTTP server started on port 80 (simulated)");
    return ESP_OK;
}

/* Simulated system functions */
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

void esp_restart(void)
{
    ESP_LOGI("SIM", "System restart (simulated)");
    exit(0);
}

/* Simulated timer functions */
int esp_timer_init(void)
{
    gettimeofday(&timer_start, NULL);
    ESP_LOGI("SIM", "Timer initialized");
    return ESP_OK;
}

/* Simulated event loop functions */
int esp_event_handler_register(void *base, int32_t id, void *handler, void *arg)
{
    ESP_LOGI("SIM", "Event handler registered (simulated)");
    return ESP_OK;
}

/* Simulated network interface functions */
int esp_netif_init(void)
{
    ESP_LOGI("SIM", "Network interface initialized (simulated)");
    return ESP_OK;
}

void* esp_netif_get_handle_from_ifkey(const char *key)
{
    ESP_LOGI("SIM", "Network interface handle retrieved (simulated)");
    return (void*)0x12345678; // Dummy handle
}

/* Include the real ESP32 Small OS main function */
extern void app_main(void);

/* Simulation signal handler */
static void sim_signal_handler(int sig)
{
    printf("\n[SIM] Received signal %d, stopping simulation...\n", sig);
    exit(0);
}

/* Main simulation */
int main(int argc, char *argv[])
{
    printf("ESP32 Small OS Simulation Runner\n");
    printf("================================\n");
    printf("Starting ESP32 Small OS in simulation mode...\n\n");
    
    /* Set up signal handlers */
    signal(SIGINT, sim_signal_handler);
    signal(SIGTERM, sim_signal_handler);
    
    /* Initialize simulation environment */
    ESP_LOGI("SIM", "Initializing simulation environment");
    
    /* Override malloc/free for tracking */
    #define malloc sim_malloc
    #define free sim_free
    
    /* Call the real ESP32 Small OS main function */
    app_main();
    
    /* This should never reach here as app_main() contains an infinite loop */
    printf("[SIM] ESP32 Small OS main function returned unexpectedly\n");
    return 0;
}

#else /* !SIMULATION */

int main(int argc, char *argv[])
{
    printf("This is a simulation-only build.\n");
    printf("Compile with -DSIMULATION flag.\n");
    return 1;
}

#endif /* SIMULATION */