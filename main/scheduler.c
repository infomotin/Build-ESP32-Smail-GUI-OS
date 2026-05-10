#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

#define TAG "SCHEDULER"
#define MAX_TASKS 16
#define IDLE_STACK_SIZE 2048
#define SCHEDULER_TIMER_PERIOD_MS 1

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

typedef struct task_cb task_cb_t;

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
    task_cb_t *next;
    task_cb_t *prev;
} task_cb_t;

static task_cb_t task_pool[MAX_TASKS];
static task_cb_t *ready_queue[TASK_PRIORITY_COUNT] = {NULL};
static task_cb_t *current_task = NULL;
static uint32_t next_task_id = 1;
static uint32_t scheduler_time = 0;
static uint32_t scheduler_tick = 0;
static bool scheduler_running = false;

static void schedule_check_triggers(void)
{
    /* Check scheduled activities - placeholder for future implementation */
}

static void scheduler_timer_callback(void *arg)
{
    scheduler_tick++;
}

void scheduler_init(void)
{
    memset(task_pool, 0, sizeof(task_pool));
    memset(ready_queue, 0, sizeof(ready_queue));
    scheduler_running = true;
    ESP_LOGI(TAG, "Scheduler initialized");
}

esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
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
    
    task->stack_ptr = (uint32_t*)malloc(stack_size);
    if (!task->stack_ptr) {
        return ESP_ERR_NO_MEM;
    }
    task->stack_top = (uint32_t)task->stack_ptr + stack_size;
    
    task->next = ready_queue[priority];
    if (ready_queue[priority]) {
        ready_queue[priority]->prev = task;
    }
    ready_queue[priority] = task;
    
    ESP_LOGI(TAG, "Created task %s (ID: %d, Priority: %d)", name, task->id, priority);
    return ESP_OK;
}

void scheduler_run_once(void)
{
    schedule_check_triggers();
    
    scheduler_time = esp_timer_get_time() / 1000;
    scheduler_tick++;
    
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
    
    if (current_task) {
        current_task->state = TASK_STATE_RUNNING;
        current_task->entry_point(current_task->parameter);
        current_task->state = TASK_STATE_READY;
        current_task->next = ready_queue[current_task->priority];
        if (current_task->next) {
            current_task->next->prev = current_task;
        }
        ready_queue[current_task->priority] = current_task;
    }
}

void scheduler_run(void)
{
    ESP_LOGI(TAG, "Scheduler started");
    
    while (scheduler_running) {
        scheduler_run_once();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Scheduler stopped");
}

void scheduler_stop(void)
{
    scheduler_running = false;
}