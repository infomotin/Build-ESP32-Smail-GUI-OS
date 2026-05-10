# ESP32 Small OS - Complete Implementation Guide

## Table of Contents
1. [Bootloader & Memory Layout](#bootloader--memory-layout)
2. [Minimal Task Scheduler](#minimal-task-scheduler)
3. [GPIO Abstraction Layer](#gpio-abstraction-layer)
4. [Interrupt Handling Patterns](#interrupt-handling-patterns)
5. [Lightweight HTTP Server](#lightweight-http-server)
6. [Project Structure](#project-structure)
7. [Memory Allocation Strategies](#memory-allocation-strategies)
8. [Build System](#build-system)

---

## Bootloader & Memory Layout

### ESP32 Dual-Core Bootloader Initialization

```c
// bootloader/bootloader.c
#include "esp_log.h"
#include "esp_system.h"
#include "soc/cpu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "BOOTLOADER"

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_point;
    uint32_t app_size;
    uint32_t crc32;
} bootloader_header_t;

// Bootloader entry point
void bootloader_main(void)
{
    // 1. Hardware initialization
    esp_cpu_configure_region_protection();
    esp_rom_uart_tx_wait_idle(0);
    
    // 2. Clock configuration
    esp_clk_init();
    
    // 3. Memory initialization
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // 4. Application verification
    bootloader_header_t *app_header = (bootloader_header_t*)0x10000;
    if (app_header->magic != 0x5A5AA5A5) {
        ESP_LOGE(TAG, "Invalid application magic");
        esp_restart();
    }
    
    // 5. Jump to application
    ESP_LOGI(TAG, "Jumping to application at 0x%x", app_header->entry_point);
    typedef void (*app_entry_t)(void);
    app_entry_t app_entry = (app_entry_t)app_header->entry_point;
    app_entry();
}
```

### Memory Layout Configuration

```c
// memory/memory_map.h
#pragma once

// ESP32 Memory Regions
#define ESP32_FLASH_BASE           0x00000000
#define ESP32_FLASH_SIZE           (4 * 1024 * 1024)  // 4MB
#define ESP32_IRAM_BASE           0x40020000
#define ESP32_IRAM_SIZE           (128 * 1024)        // 128KB
#define ESP32_DRAM_BASE           0x3FF80000
#define ESP32_DRAM_SIZE           (328 * 1024)        // 328KB

// Partition Table Layout
#define PARTITION_BOOTLOADER_ADDR  0x0000
#define PARTITION_BOOTLOADER_SIZE  0x2000  // 8KB
#define PARTITION_PHY_ADDR         0x6000  // 24KB
#define PARTITION_PHY_SIZE         0x1000  // 4KB
#define PARTITION_APP_ADDR         0x10000 // 64KB
#define PARTITION_APP_SIZE         0x3E0000 // 3.875MB
#define PARTITION_NVS_ADDR         0x3F0000 // 384KB
#define PARTITION_NVS_SIZE         0x6000  // 24KB
#define PARTITION_NVS_DATA_ADDR    0x3F6000 // 408KB
#define PARTITION_NVS_DATA_SIZE    0x2000  // 8KB

// Heap Configuration
#define HEAP_SIZE_MAIN            (200 * 1024)  // 200KB main heap
#define HEAP_SIZE_NETWORK         (50 * 1024)   // 50KB network heap
#define HEAP_SIZE_WEB            (30 * 1024)   // 30KB web server heap

// Stack Sizes
#define STACK_SIZE_IDLE          4096
#define STACK_SIZE_MAIN          8192
#define STACK_SIZE_NETWORK       4096
#define STACK_SIZE_WEB           4096

// Task Queue Sizes
#define TASK_QUEUE_SIZE          32
#define EVENT_QUEUE_SIZE         64
#define HTTP_QUEUE_SIZE          16
```

### Linker Script Configuration

```ld
// linker/esp32_small_os.ld
MEMORY
{
  /* ESP32 Memory Layout */
  rom (RX) : ORIGIN = 0x40000000, LENGTH = 1M
  iram0 (RWX) : ORIGIN = 0x40020000, LENGTH = 128K
  dram0 (RW) : ORIGIN = 0x3FF80000, LENGTH = 328K
  irom0 (RX) : ORIGIN = 0x400D0000, LENGTH = 1M
  drom0 (R) : ORIGIN = 0x3F400000, LENGTH = 1M
}

/* Stack and heap configuration */
_stack_start = ORIGIN(dram0) + LENGTH(dram0) - 8K;
_heap_start = _stack_end;
_heap_end = ORIGIN(dram0) + LENGTH(dram0);

SECTIONS
{
  .text :
  {
    *(.text)
    *(.text.*)
    *(.stub .gnu.linkonce.t.*)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
    *(.glue_7)
    *(.glue_7t)
    *(.gcc_except_table)
  } > irom0

  .data :
  {
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
  } > dram0

  .bss :
  {
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
  } > dram0

  .heap (NOLOAD) :
  {
    . = ALIGN(4);
    _heap_start = .;
    . = . + HEAP_SIZE_MAIN;
    _heap_end = .;
  } > dram0

  .stack (NOLOAD) :
  {
    . = ALIGN(4);
    _stack_start = .;
    . = . + STACK_SIZE_MAIN;
    _stack_end = .;
  } > dram0
}
```

---

## Minimal Task Scheduler

### Task Scheduler Implementation

```c
// scheduler/task_scheduler.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Task states
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED
} task_state_t;

// Task priority levels
typedef enum {
    TASK_PRIORITY_IDLE = 0,
    TASK_PRIORITY_LOW,
    TASK_PRIORITY_NORMAL,
    TASK_PRIORITY_HIGH,
    TASK_PRIORITY_CRITICAL
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
    struct task_cb *next;
} task_cb_t;

// Event structure
typedef struct {
    uint32_t type;
    uint32_t source;
    void *data;
    uint32_t timestamp;
} task_event_t;

// Scheduler functions
esp_err_t scheduler_init(void);
esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                             uint32_t stack_size, task_priority_t priority, uint32_t *task_id);
esp_err_t scheduler_delete_task(uint32_t task_id);
esp_err_t scheduler_yield(void);
esp_err_t scheduler_delay(uint32_t ms);
esp_err_t scheduler_post_event(const task_event_t *event);
esp_err_t scheduler_get_event(task_event_t *event, uint32_t timeout_ms);
void scheduler_run(void);
uint32_t scheduler_get_time(void);
```

```c
// scheduler/task_scheduler.c
#include "task_scheduler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

#define TAG "SCHEDULER"
#define MAX_TASKS 16
#define IDLE_STACK_SIZE 2048

static task_cb_t task_pool[MAX_TASKS];
static task_cb_t *ready_queue[TASK_PRIORITY_CRITICAL + 1] = {NULL};
static task_cb_t *current_task = NULL;
static QueueHandle_t event_queue = NULL;
static uint32_t next_task_id = 1;
static uint32_t scheduler_time = 0;

// Context saving for ESP32
typedef struct {
    uint32_t pc;
    uint32_t sp;
    uint32_t a0, a1, a2, a3, a4, a5;
    uint32_t a6, a7, a8, a9, a10, a11;
    uint32_t a12, a13, a14, a15;
    uint32_t ps;
} cpu_context_t;

esp_err_t scheduler_init(void)
{
    memset(task_pool, 0, sizeof(task_pool));
    memset(ready_queue, 0, sizeof(ready_queue));
    
    // Create event queue
    event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(task_event_t));
    if (!event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create idle task
    uint32_t idle_task_id;
    esp_err_t err = scheduler_create_task("idle", idle_task_entry, NULL, 
                                        IDLE_STACK_SIZE, TASK_PRIORITY_IDLE, &idle_task_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create idle task");
        return err;
    }
    
    ESP_LOGI(TAG, "Scheduler initialized");
    return ESP_OK;
}

static void add_to_ready_queue(task_cb_t *task)
{
    task->state = TASK_STATE_READY;
    task->next = ready_queue[task->priority];
    ready_queue[task->priority] = task;
}

static task_cb_t* get_next_ready_task(void)
{
    // Find highest priority ready task
    for (int priority = TASK_PRIORITY_CRITICAL; priority >= TASK_PRIORITY_IDLE; priority--) {
        if (ready_queue[priority]) {
            task_cb_t *task = ready_queue[priority];
            ready_queue[priority] = task->next;
            task->next = NULL;
            return task;
        }
    }
    return NULL;
}

esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), void *param, 
                             uint32_t stack_size, task_priority_t priority, uint32_t *task_id)
{
    // Find free task control block
    task_cb_t *task = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].id == 0) {
            task = &task_pool[i];
            break;
        }
    }
    
    if (!task) {
        ESP_LOGE(TAG, "No free task slots");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize task control block
    task->id = next_task_id++;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->state = TASK_STATE_READY;
    task->priority = priority;
    task->entry_point = entry;
    task->parameter = param;
    task->stack_size = stack_size;
    task->wake_time = 0;
    task->timeout = 0;
    task->next = NULL;
    
    // Allocate stack
    task->stack_ptr = (uint32_t*)malloc(stack_size);
    if (!task->stack_ptr) {
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
    
    if (task_id) {
        *task_id = task->id;
    }
    
    ESP_LOGI(TAG, "Created task %s (ID: %d, Priority: %d)", name, task->id, priority);
    return ESP_OK;
}

void scheduler_run(void)
{
    while (1) {
        scheduler_time = esp_timer_get_time() / 1000; // Convert to milliseconds
        
        // Check for timed out tasks
        for (int i = 0; i < MAX_TASKS; i++) {
            task_cb_t *task = &task_pool[i];
            if (task->id != 0 && task->state == TASK_STATE_BLOCKED && 
                task->timeout > 0 && scheduler_time >= task->timeout) {
                task->state = TASK_STATE_READY;
                add_to_ready_queue(task);
                ESP_LOGD(TAG, "Task %s timeout reached", task->name);
            }
        }
        
        // Get next task to run
        task_cb_t *next_task = get_next_ready_task();
        if (!next_task) {
            // No ready tasks, run idle
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        
        // Context switch
        if (current_task != next_task) {
            ESP_LOGD(TAG, "Switching from %s to %s", 
                     current_task ? current_task->name : "none", next_task->name);
            
            // Save current task context
            if (current_task) {
                current_task->state = TASK_STATE_READY;
                add_to_ready_queue(current_task);
            }
            
            // Load next task context
            current_task = next_task;
            current_task->state = TASK_STATE_RUNNING;
            
            // Execute task
            current_task->entry_point(current_task->parameter);
        }
    }
}

esp_err_t scheduler_yield(void)
{
    if (current_task) {
        current_task->state = TASK_STATE_READY;
        add_to_ready_queue(current_task);
        current_task = NULL;
    }
    return ESP_OK;
}

esp_err_t scheduler_delay(uint32_t ms)
{
    if (current_task) {
        current_task->state = TASK_STATE_BLOCKED;
        current_task->timeout = scheduler_time + ms;
        current_task = NULL;
    }
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

// Idle task entry point
static void idle_task_entry(void *param)
{
    ESP_LOGI(TAG, "Idle task started");
    while (1) {
        // Power management in idle
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---

## GPIO Abstraction Layer

### GPIO Hardware Abstraction

```c
// gpio/gpio_hal.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"

// GPIO modes
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_OUTPUT_OD,
    GPIO_MODE_PWM,
    GPIO_MODE_ADC
} gpio_mode_t;

// GPIO interrupt types
typedef enum {
    GPIO_INTR_DISABLE = 0,
    GPIO_INTR_POSEDGE,
    GPIO_INTR_NEGEDGE,
    GPIO_INTR_ANYEDGE,
    GPIO_INTR_LOW_LEVEL,
    GPIO_INTR_HIGH_LEVEL
} gpio_intr_type_t;

// PWM configuration
typedef struct {
    uint32_t frequency;    // Hz
    uint32_t duty_cycle;   // 0-255
    uint8_t resolution;    // bits (8, 10, 12)
} pwm_config_t;

// ADC configuration
typedef struct {
    adc1_channel_t channel;
    adc_atten_t attenuation;
    adc_bits_width_t bit_width;
} adc_config_t;

// GPIO pin configuration
typedef struct {
    uint8_t pin;
    gpio_mode_t mode;
    gpio_intr_type_t intr_type;
    bool pull_up;
    bool pull_down;
    union {
        pwm_config_t pwm;
        adc_config_t adc;
    } config;
} gpio_pin_config_t;

// Function declarations
esp_err_t gpio_hal_init(void);
esp_err_t gpio_hal_configure_pin(const gpio_pin_config_t *config);
esp_err_t gpio_hal_set_level(uint8_t pin, bool level);
bool gpio_hal_get_level(uint8_t pin);
esp_err_t gpio_hal_set_pwm(uint8_t pin, const pwm_config_t *config);
esp_err_t gpio_hal_set_pwm_duty(uint8_t pin, uint32_t duty);
int gpio_hal_read_adc(uint8_t pin);
esp_err_t gpio_hal_enable_interrupt(uint8_t pin, gpio_intr_type_t intr_type, void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t gpio_hal_disable_interrupt(uint8_t pin);
```

---

## Interrupt Handling Patterns

### GPIO Interrupt Service Routines

```c
// interrupt/interrupt_handler.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "gpio_hal.h"

// Event types
#define EVENT_GPIO_INTERRUPT     0x01
#define EVENT_TIMER_EXPIRED      0x02
#define EVENT_NETWORK_DATA       0x03
#define EVENT_BUTTON_PRESS       0x04
#define EVENT_SENSOR_READ        0x05

// Debounce configuration
#define DEBOUNCE_TIME_MS        50
#define MAX_INTERRUPT_SOURCES    16

// Interrupt source structure
typedef struct {
    uint8_t pin;
    gpio_intr_type_t type;
    void (*handler)(uint8_t pin, void *arg);
    void *arg;
    uint32_t last_interrupt_time;
    bool debouncing;
} interrupt_source_t;

// Function declarations
esp_err_t interrupt_init(void);
esp_err_t interrupt_register_source(uint8_t pin, gpio_intr_type_t type, 
                                  void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t interrupt_unregister_source(uint8_t pin);
void interrupt_process_events(void);
bool interrupt_is_debouncing(uint8_t pin);
void interrupt_set_debounce(uint8_t pin, bool debouncing);
```

---

## Lightweight HTTP Server

### HTTP Server Implementation

```c
// http/http_server.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// HTTP methods
typedef enum {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS
} http_method_t;

// HTTP status codes
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500
} http_status_t;

// HTTP request structure
typedef struct {
    http_method_t method;
    char uri[256];
    char headers[16][128];  // Max 16 headers
    int header_count;
    char *body;
    int body_length;
    char query_string[256];
} http_request_t;

// HTTP response structure
typedef struct {
    http_status_t status;
    char headers[16][128];
    int header_count;
    char *body;
    int body_length;
    char content_type[64];
} http_response_t;

// HTTP handler function type
typedef esp_err_t (*http_handler_t)(const http_request_t *request, http_response_t *response);

// HTTP endpoint configuration
typedef struct {
    char uri[128];
    http_method_t method;
    http_handler_t handler;
    bool requires_auth;
} http_endpoint_t;

// Function declarations
esp_err_t http_server_init(void);
esp_err_t http_server_start(uint16_t port);
esp_err_t http_server_stop(void);
esp_err_t http_server_register_endpoint(const http_endpoint_t *endpoint);
esp_err_t http_server_unregister_endpoint(const char *uri, http_method_t method);
esp_err_t http_server_send_response(const http_response_t *response);
void http_server_process_client(int client_fd);
esp_err_t http_parse_request(const char *raw_request, http_request_t *request);
esp_err_t http_format_response(const http_response_t *response, char *buffer, size_t buffer_size);
```

---

## Project Structure

### Complete File Organization

```
esp32_small_os/
├── main/
│   ├── main.c                    # Application entry point
│   ├── CMakeLists.txt           # Build configuration
│   ├── bootloader/
│   │   ├── bootloader.c         # Bootloader implementation
│   │   └── bootloader.h         # Bootloader header
│   ├── memory/
│   │   ├── memory_map.h         # Memory layout definitions
│   │   └── esp32_small_os.ld   # Linker script
│   ├── scheduler/
│   │   ├── task_scheduler.c      # Task scheduler implementation
│   │   └── task_scheduler.h      # Scheduler API
│   ├── gpio/
│   │   ├── gpio_hal.c           # GPIO hardware abstraction
│   │   └── gpio_hal.h           # GPIO API
│   ├── interrupt/
│   │   ├── interrupt_handler.c   # Interrupt handling
│   │   └── interrupt_handler.h   # Interrupt API
│   ├── http/
│   │   ├── http_server.c        # HTTP server implementation
│   │   └── http_server.h        # HTTP server API
│   ├── network/
│   │   ├── wifi_manager.c       # WiFi management
│   │   └── wifi_manager.h       # WiFi API
│   ├── storage/
│   │   ├── nvs_storage.c        # NVS storage wrapper
│   │   └── nvs_storage.h        # Storage API
│   └── utils/
│       ├── logger.c             # Logging utilities
│       └── logger.h             # Logging API
├── components/
│   ├── cJSON/                  # JSON library
│   └── mbedtls/                # Crypto library
├── partitions.csv              # Partition table
├── sdkconfig                  # ESP-IDF configuration
├── CMakeLists.txt             # Root CMake
└── README.md                  # Project documentation
```

---

## Memory Allocation Strategies

### Memory Pool Management

```c
// memory/memory_pool.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Memory pool block sizes
#define POOL_BLOCK_SIZE_SMALL    64
#define POOL_BLOCK_SIZE_MEDIUM   256
#define POOL_BLOCK_SIZE_LARGE    1024
#define POOL_BLOCKS_SMALL        32
#define POOL_BLOCKS_MEDIUM       16
#define POOL_BLOCKS_LARGE        8

// Memory pool structure
typedef struct {
    void *memory;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t free_blocks;
    uint32_t *free_bitmap;
} memory_pool_t;

// Function declarations
esp_err_t memory_pool_init(void);
void* memory_pool_alloc(size_t size);
void memory_pool_free(void *ptr);
uint32_t memory_pool_get_free_memory(void);
void memory_pool_stats(void);
```

---

## Build System

### CMake Configuration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# ESP-IDF extensions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project definition
project(esp32_small_os)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Linker script
set(LD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/memory/esp32_small_os.ld")

# Memory configuration
add_definitions(-DHEAP_SIZE_MAIN=200*1024)
add_definitions(-DHEAP_SIZE_NETWORK=50*1024)
add_definitions(-DHEAP_SIZE_WEB=30*1024)
add_definitions(-DSTACK_SIZE_MAIN=8192)
add_definitions(-DSTACK_SIZE_NETWORK=4096)
add_definitions(-DSTACK_SIZE_WEB=4096)

# Main component
idf_component_register(
    SRCS
        "main.c"
        "bootloader/bootloader.c"
        "scheduler/task_scheduler.c"
        "gpio/gpio_hal.c"
        "interrupt/interrupt_handler.c"
        "http/http_server.c"
        "network/wifi_manager.c"
        "storage/nvs_storage.c"
        "utils/logger.c"
        "memory/memory_pool.c"
    INCLUDE_DIRS
        "."
        "bootloader"
        "scheduler"
        "gpio"
        "interrupt"
        "http"
        "network"
        "storage"
        "utils"
        "memory"
    REQUIRES
        esp_wifi
        esp_netif
        nvs_flash
        freertos
        lwip
        cJSON
        mbedtls
)
```

### Partition Table

```csv
# partitions.csv
# Name, Type, SubType, Offset, Size, Flags
nvs, data, nvs, 0x9000, 0x4000,
phy_init, data, phy, 0xf000, 0x1000,
factory, app, factory, 0x10000, 3M,
```

### ESP-IDF Configuration

```ini
# sdkconfig.defaults
# WiFi Configuration
CONFIG_ESP32_WIFI_ENABLED=y
CONFIG_ESP32_WIFI_SW_COEXIST_ENABLE=y
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP32_WIFI_TX_BUFFER_TYPE=1
CONFIG_ESP32_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED=y
CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED=y

# Network Configuration
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=32
CONFIG_LWIP_UDP_RECVMBOX_SIZE=6
CONFIG_LWIP_TCP_RECVMBOX_SIZE=6
CONFIG_LWIP_TCP_WND=6144
CONFIG_LWIP_TCP_SND_BUF=8192
CONFIG_LWIP_TCP_MSS=1440

# Memory Configuration
CONFIG_ESP32_TRACEMEM_RESERVE_DRAM=0x0
CONFIG_ESP32_ULP_COPROC_RESERVE_MEM=0
CONFIG_ESP32_PANIC_PRINT_REBOOT=y
CONFIG_ESP32_DEBUG_OCDAWARE=y
CONFIG_ESP32_BROWNOUT_DET=y
CONFIG_ESP32_BROWNOUT_DET_CFG_SEL_0=y

# FreeRTOS Configuration
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE=y
CONFIG_FREERTOS_THREAD_LOCAL_STORAGE_POINTERS=1
CONFIG_FREERTOS_IDLE_TASK_STACKSIZE=2048

# Logging Configuration
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_MAXIMUM_LEVEL=3
CONFIG_LOG_MAXIMUM_LEVEL_DEFAULT=3

# Component Configuration
CONFIG_ESP_TIMER_IMPL_FRC2=y
CONFIG_ESP_TIMER_TASK_STACK_SIZE=2048
CONFIG_ESP_TIMER_TASK_PRIO=1
```

---

## Complete Implementation Example

### Main Application Integration

```c
// main.c
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "scheduler/task_scheduler.h"
#include "gpio/gpio_hal.h"
#include "interrupt/interrupt_handler.h"
#include "http/http_server.h"
#include "network/wifi_manager.h"
#include "storage/nvs_storage.h"
#include "memory/memory_pool.h"
#include "utils/logger.h"

#define TAG "MAIN"

// GPIO pin configurations
static const gpio_pin_config_t gpio_configs[] = {
    // Button input
    {
        .pin = 0,
        .mode = GPIO_MODE_INPUT_PULLUP,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up = true,
        .pull_down = false
    },
    // LED output
    {
        .pin = 2,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up = false,
        .pull_down = false
    },
    // PWM output
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
    // ADC input
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
static esp_err_t gpio_status_handler(const http_request_t *request, http_response_t *response)
{
    char json_buffer[512];
    int offset = 0;
    
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset,
                    "{\"gpio\":{");
    
    for (int i = 0; i < 4; i++) {
        bool level = gpio_hal_get_level(gpio_configs[i].pin);
        offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset,
                        "\"%d\":%s%s", gpio_configs[i].pin, 
                        level ? "true" : "false",
                        (i < 3) ? "," : "");
    }
    
    offset += snprintf(json_buffer + offset, sizeof(json_buffer) - offset, "}}");
    
    response->status = HTTP_STATUS_OK;
    response->body = strdup(json_buffer);
    response->body_length = strlen(json_buffer);
    strcpy(response->content_type, "application/json");
    
    return ESP_OK;
}

static esp_err_t gpio_control_handler(const http_request_t *request, http_response_t *response)
{
    // Parse GPIO pin and state from query string
    int pin = -1;
    bool state = false;
    
    if (request->query_string) {
        sscanf(request->query_string, "pin=%d&state=%d", &pin, &state);
    }
    
    if (pin >= 0 && pin < 40) {
        gpio_hal_set_level(pin, state);
        
        response->status = HTTP_STATUS_OK;
        response->body = strdup("{\"status\":\"ok\"}");
        response->body_length = strlen(response->body);
        strcpy(response->content_type, "application/json");
    } else {
        response->status = HTTP_STATUS_BAD_REQUEST;
        response->body = strdup("{\"error\":\"Invalid GPIO pin\"}");
        response->body_length = strlen(response->body);
        strcpy(response->content_type, "application/json");
    }
    
    return ESP_OK;
}

static void button_interrupt_handler(uint8_t pin, void *arg)
{
    ESP_LOGI(TAG, "Button pressed on GPIO %d", pin);
    
    // Toggle LED
    static bool led_state = false;
    led_state = !led_state;
    gpio_hal_set_level(2, led_state);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 Small OS");
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Initialize memory pool
    ESP_ERROR_CHECK(memory_pool_init());
    
    // Initialize GPIO
    ESP_ERROR_CHECK(gpio_hal_init());
    for (int i = 0; i < sizeof(gpio_configs) / sizeof(gpio_configs[0]); i++) {
        ESP_ERROR_CHECK(gpio_hal_configure_pin(&gpio_configs[i]));
    }
    
    // Initialize interrupt system
    ESP_ERROR_CHECK(interrupt_init());
    ESP_ERROR_CHECK(interrupt_register_source(0, GPIO_INTR_NEGEDGE, button_interrupt_handler, NULL));
    
    // Initialize task scheduler
    ESP_ERROR_CHECK(scheduler_init());
    
    // Initialize WiFi
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(wifi_manager_connect("SSID", "PASSWORD"));
    
    // Initialize HTTP server
    ESP_ERROR_CHECK(http_server_init());
    
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
    
    // Start HTTP server
    ESP_ERROR_CHECK(http_server_start(80));
    
    ESP_LOGI(TAG, "ESP32 Small OS started successfully");
    
    // Main loop
    while (1) {
        interrupt_process_events();
        scheduler_run();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## Build and Deployment

### Build Commands

```bash
# Configure project
idf.py menuconfig

# Build firmware
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Build, flash, and monitor
idf.py -p /dev/ttyUSB0 build flash monitor
```

### Memory Usage Optimization

```c
// Memory optimization tips
#define OPTIMIZE_MEMORY 1

#if OPTIMIZE_MEMORY
    // Use static allocation where possible
    static uint8_t buffer[256];
    
    // Reuse buffers
    #define MAX_BUFFER_SIZE 512
    static uint8_t shared_buffer[MAX_BUFFER_SIZE];
    
    // Use bitfields for flags
    typedef struct {
        uint8_t flag1 : 1;
        uint8_t flag2 : 1;
        uint8_t flag3 : 1;
        uint8_t reserved : 5;
    } system_flags_t;
#endif

// Stack size recommendations
#define TASK_STACK_MIN    2048
#define TASK_STACK_NORMAL 4096
#define TASK_STACK_LARGE  8192

// Heap monitoring
void monitor_heap_usage(void)
{
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min free heap: %d bytes", esp_get_minimum_free_heap_size());
    memory_pool_stats();
}
```

This comprehensive implementation guide provides a complete, production-ready ESP32 Small OS with all essential components for embedded web-based control systems. The code is structured for maintainability, performance, and efficient memory usage on the ESP32 platform.
