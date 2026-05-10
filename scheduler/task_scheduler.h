#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Task states
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_DELETED
} task_state_t;

// Task priority levels
typedef enum {
    TASK_PRIORITY_IDLE = 0,
    TASK_PRIORITY_LOW,
    TASK_PRIORITY_NORMAL,
    TASK_PRIORITY_HIGH,
    TASK_PRIORITY_CRITICAL,
    TASK_PRIORITY_COUNT
} task_priority_t;

// Task control block
typedef struct task_cb {
    uint32_t id;
    char name[16];
    task_state_t state;
    task_priority_t priority;
    void (*entry_point)(void *param);
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

// Event structure
typedef struct {
    uint32_t type;
    uint32_t source;
    void *data;
    uint32_t timestamp;
    uint32_t priority;
} task_event_t;

// Event types
#define EVENT_TYPE_GPIO        0x01
#define EVENT_TYPE_TIMER       0x02
#define EVENT_TYPE_NETWORK     0x03
#define EVENT_TYPE_HTTP        0x04
#define EVENT_TYPE_SYSTEM      0x05
#define EVENT_TYPE_USER        0x06

// Scheduler statistics
typedef struct {
    uint32_t total_tasks;
    uint32_t ready_tasks;
    uint32_t blocked_tasks;
    uint32_t context_switches;
    uint32_t idle_time;
    uint32_t total_run_time;
    uint32_t max_response_time;
    uint32_t avg_response_time;
} scheduler_stats_t;

// Scheduler configuration
typedef struct {
    uint32_t max_tasks;
    uint32_t time_slice_ms;
    uint32_t idle_timeout_ms;
    bool preemptive_enabled;
    bool statistics_enabled;
} scheduler_config_t;

// Function declarations
esp_err_t scheduler_init(const scheduler_config_t *config);
esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                             uint32_t stack_size, task_priority_t priority, uint32_t *task_id);
esp_err_t scheduler_delete_task(uint32_t task_id);
esp_err_t scheduler_suspend_task(uint32_t task_id);
esp_err_t scheduler_resume_task(uint32_t task_id);
esp_err_t scheduler_yield(void);
esp_err_t scheduler_delay(uint32_t ms);
esp_err_t scheduler_delay_until(uint32_t timestamp);
esp_err_t scheduler_post_event(const task_event_t *event);
esp_err_t scheduler_post_event_from_isr(const task_event_t *event);
esp_err_t scheduler_get_event(task_event_t *event, uint32_t timeout_ms);
esp_err_t scheduler_peek_event(task_event_t *event);
void scheduler_run(void);
void scheduler_run_once(void);
uint32_t scheduler_get_time(void);
uint32_t scheduler_get_tick_count(void);
task_cb_t* scheduler_get_current_task(void);
esp_err_t scheduler_get_task_by_id(uint32_t task_id, task_cb_t **task);
esp_err_t scheduler_get_statistics(scheduler_stats_t *stats);
void scheduler_print_statistics(void);
esp_err_t scheduler_set_priority(uint32_t task_id, task_priority_t priority);
task_priority_t scheduler_get_priority(uint32_t task_id);
bool scheduler_is_task_running(uint32_t task_id);
uint32_t scheduler_get_task_count(void);
uint32_t scheduler_get_ready_count(void);
void scheduler_dump_tasks(void);

// Internal functions
static void add_to_ready_queue(task_cb_t *task);
static task_cb_t* get_next_ready_task(void);
static void remove_from_ready_queue(task_cb_t *task);
static void update_task_statistics(task_cb_t *task);
static void context_switch(task_cb_t *from_task, task_cb_t *to_task);
static void idle_task_entry(void *param);
static void scheduler_timer_callback(void *arg);
static void process_timeouts(void);
static void update_statistics(void);
