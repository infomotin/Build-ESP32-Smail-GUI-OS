# ESP32 Small OS Implementation Guide

## Table of Contents
1. [Bootloader and Memory Layout](#bootloader-and-memory-layout)
2. [Non-Preemptive Task Scheduler Details](#non-preemptive-task-scheduler-details)
3. [GPIO Abstraction Layer](#gpio-abstraction-layer)
4. [Interrupt Handling Patterns](#interrupt-handling-patterns)
5. [HTTP Server Foundation](#http-server-foundation)
6. [Project Structure](#project-structure)
7. [Memory Allocation Strategies](#memory-allocation-strategies)
8. [Build System](#build-system)
9. [Simulation Mode](#simulation-mode)

---

## Bootloader and Memory Layout

### Memory Map

The ESP32 Small OS uses a segmented memory layout optimized for embedded applications:

| Address Range | Size | Description |
|--------------|------|-------------|
| 0x00000000 | 192KB | SRAM0/SRAM1 (Data RAM) |
| 0x3F800000 | 64KB | SRAM2 (Instruction RAM) |
| 0x3FFB0000 | 128KB | SRAM3 (Data/Instruction RAM) |
| 0x40000000 | - | External Flash |

### Bootloader Sequence

The bootloader (`bootloader/bootloader.c`) performs the following steps:

1. **Hardware Initialization**
   ```c
   esp_cpu_configure_region_protection();
   esp_rom_uart_tx_wait_idle(0);
   ```

2. **Clock Configuration**
   ```c
   esp_clk_init();
   ```

3. **NVS Flash Initialization**
   ```c
   esp_err_t err = nvs_flash_init();
   ```

4. **Application Verification**
   - Validates magic number: `0x5A5AA5A5`
   - Checks CRC32 checksum
   - Verifies entry point

5. **Jump to Application**
   ```c
   typedef void (*app_entry_t)(void);
   app_entry_t app_entry = (app_entry_t)app_header->entry_point;
   app_entry();
   ```

### Application Header Structure

```c
typedef struct {
    uint32_t magic;          // 0x5A5AA5A5
    uint32_t version;        // Application version
    uint32_t entry_point;    // Entry point address
    uint32_t app_size;       // Size in bytes
    uint32_t crc32;          // CRC32 checksum
} bootloader_header_t;
```

---

## Non-Preemptive Task Scheduler Details

### Architecture

The Small OS scheduler uses a non-preemptive cooperative scheduling approach with priority-based ready queues.

### Task Control Block

```c
typedef struct task_cb {
    uint32_t id;              // Unique task identifier
    char name[32];            // Task name
    task_state_t state;       // READY, RUNNING, BLOCKED, DELETED
    task_priority_t priority; // IDLE, LOW, NORMAL, HIGH, CRITICAL
    task_entry_t entry_point; // Task function pointer
    void *parameter;          // Parameter passed to task
    uint32_t stack_size;      // Stack size in bytes
    uint32_t *stack_ptr;      // Stack pointer
    uint32_t stack_top;       // Stack top address
    uint32_t wake_time;       // Wake time for delayed tasks
    uint32_t timeout;         // Timeout for blocked tasks
    uint32_t run_count;       // Execution counter
    uint32_t total_run_time;  // Total execution time
    task_cb_t *next;          // Next task in queue
    task_cb_t *prev;          // Previous task in queue
} task_cb_t;
```

### Priority Queue Structure

Tasks are organized in priority-based linked lists:

```
Critical Priority Queue -> [Task] -> [Task] -> NULL
High Priority Queue     -> [Task] -> NULL
Normal Priority Queue   -> [Task] -> [Task] -> [Task] -> NULL
Low Priority Queue      -> NULL
Idle Priority Queue     -> [Idle Task] -> NULL
```

### Scheduler API

```c
// Initialize scheduler
esp_err_t scheduler_init(const scheduler_config_t *config);

// Create a new task
esp_err_t scheduler_create_task(const char *name, void (*entry)(void*), 
                                void *param, uint32_t stack_size, 
                                task_priority_t priority, uint32_t *task_id);

// Run one scheduler iteration
void scheduler_run_once(void);

// Run scheduler main loop
void scheduler_run(void);

// Yield to next task
esp_err_t scheduler_yield(void);

// Delay current task
esp_err_t scheduler_delay(uint32_t ms);
```

### Scheduling Algorithm

1. **Timer Callback** (`scheduler_timer_callback`): Increments tick counter
2. **Schedule Check** (`schedule_check_triggers`): Checks scheduled activities
3. **Timeout Processing**: Moves blocked tasks to ready queue when timeout expires
4. **Task Selection**: Picks highest priority ready task
5. **Context Switch**: Saves/loads task context
6. **Task Execution**: Runs selected task

### Time Slice Management

```c
#define TIME_SLICE_MS 10
#define SCHEDULER_TIMER_PERIOD_MS 1

if (scheduler_config.preemptive_enabled && run_time >= scheduler_config.time_slice_ms) {
    current_task->state = TASK_STATE_READY;
    add_to_ready_queue(current_task);
    current_task = NULL;
}
```

---

## GPIO Abstraction Layer

### Supported Pin Modes

| Mode | Description |
|------|-------------|
| GPIO_MODE_INPUT | Basic input |
| GPIO_MODE_INPUT_PULLUP | Input with pull-up resistor |
| GPIO_MODE_INPUT_PULLDOWN | Input with pull-down resistor |
| GPIO_MODE_OUTPUT | Basic output |
| GPIO_MODE_OUTPUT_OD | Open-drain output |
| GPIO_MODE_PWM | PWM output via LEDC |
| GPIO_MODE_ADC | Analog input |
| GPIO_MODE_DAC | DAC output |
| GPIO_MODE_I2C | I2C bus |
| GPIO_MODE_SPI | SPI bus |
| GPIO_MODE_UART | UART interface |
| GPIO_MODE_RMT | Remote control |

### API Functions

#### Initialization
```c
esp_err_t gpio_hal_init(void);
```

#### Pin Configuration
```c
esp_err_t gpio_hal_configure_pin(const gpio_pin_config_t *config);
esp_err_t gpio_hal_set_level(uint8_t pin, bool level);
bool gpio_hal_get_level(uint8_t pin);
```

#### PWM Control
```c
esp_err_t gpio_hal_set_pwm(uint8_t pin, const pwm_config_t *config);
esp_err_t gpio_hal_set_pwm_duty(uint8_t pin, uint32_t duty_cycle);
```

#### ADC Reading
```c
int gpio_hal_read_adc(uint8_t pin);
esp_err_t gpio_hal_read_adc_voltage(uint8_t pin, float *voltage);
```

#### Interrupt Handling
```c
esp_err_t gpio_hal_enable_interrupt(uint8_t pin, gpio_intr_type_t intr_type,
                                    void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t gpio_hal_disable_interrupt(uint8_t pin);
```

### Interrupt Service Routine

```c
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    
    gpio_stats.total_interrupts++;
    
    if (gpio_num < MAX_GPIO_PINS) {
        pin_states[gpio_num].level = gpio_get_level(gpio_num);
        pin_states[gpio_num].last_change_time = esp_timer_get_time() / 1000;
        pin_states[gpio_num].interrupt_count++;
        
        if (gpio_handlers[gpio_num]) {
            gpio_handlers[gpio_num](gpio_num, gpio_handler_args[gpio_num]);
        }
    }
}
```

---

## Interrupt Handling Patterns

### Interrupt Sources

| Type | Source | Handler |
|------|--------|---------|
| GPIO | Pin change | `gpio_isr_handler` |
| Timer | Hardware timer | `timer_isr_handler` |
| Network | WiFi events | `network_isr_handler` |

### Data Structures

```c
typedef struct {
    uint8_t pin;              // GPIO pin number
    gpio_intr_type_t type;    // Interrupt type
    void (*handler)(uint8_t pin, void *arg); // User handler
    void *arg;                // User argument
    uint32_t last_interrupt_time; // Last interrupt timestamp
    uint32_t interrupt_count; // Total interrupt count
    bool debouncing;          // Debounce flag
    uint8_t debounce_state;   // Debounce state machine
    uint8_t stable_count;     // Stable sample count
    bool enabled;             // Source enabled flag
    int priority;             // Event priority
} interrupt_source_t;
```

### ISR-to-Task Communication

```c
// ISR queue for high-priority events
static volatile interrupt_event_t isr_queue[ISR_QUEUE_SIZE];
static volatile uint8_t isr_queue_head = 0;
static volatile uint8_t isr_queue_tail = 0;

static void IRAM_ATTR queue_event_from_isr(const interrupt_event_t *event)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    
    uint8_t next_head = (isr_queue_head + 1) % ISR_QUEUE_SIZE;
    if (next_head != isr_queue_tail) {
        isr_queue[isr_queue_head] = *event;
        isr_queue_head = next_head;
        xSemaphoreGiveFromISR(isr_queue_sem, &higher_priority_task_woken);
    } else {
        interrupt_stats.missed_events++;
    }
    
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
```

### Debounce Pattern

```c
typedef enum {
    DEBOUNCE_STATE_IDLE = 0,
    DEBOUNCE_STATE_FIRST_SAMPLE = 1,
    DEBOUNCE_STATE_STABLE = 2
} debounce_state_t;

static bool IRAM_ATTR debounce_interrupt(uint8_t pin, bool level)
{
    interrupt_source_t *source = find_source_by_pin(pin);
    if (!source) return true;
    
    switch (source->debounce_state) {
        case DEBOUNCE_STATE_IDLE:
            source->debounce_state = DEBOUNCE_STATE_FIRST_SAMPLE;
            source->stable_count = 1;
            return false;
            
        case DEBOUNCE_STATE_FIRST_SAMPLE:
            if (level == gpio_get_level(pin)) {
                source->stable_count++;
                if (source->stable_count >= DEBOUNCE_STABLE_COUNT) {
                    source->debounce_state = DEBOUNCE_STATE_STABLE;
                    return true;
                }
            } else {
                source->debounce_state = DEBOUNCE_STATE_IDLE;
            }
            return false;
            
        case DEBOUNCE_STATE_STABLE:
            if (level != gpio_get_level(pin)) {
                source->debounce_state = DEBOUNCE_STATE_IDLE;
                return false;
            }
            return true;
            
        default:
            source->debounce_state = DEBOUNCE_STATE_IDLE;
            return false;
    }
}
```

---

## HTTP Server Foundation

### Architecture

The HTTP server uses a socket-based approach with lwIP:

```
Client Request
      ↓
Socket Accept
      ↓
http_parse_request()
      ↓
Handler Matching
      ↓
http_format_response()
      ↓
Client Response
```

### Core Components

#### Endpoint Registration
```c
typedef struct {
    const char *uri;           // URI pattern
    http_method_t method;      // HTTP method
    http_handler_t handler;    // Handler function
} http_endpoint_t;

esp_err_t http_server_register_endpoint(const http_endpoint_t *endpoint);
```

#### Request Processing
```c
typedef struct {
    char uri[128];              // Request URI
    http_method_t method;       // HTTP method
    char version[16];           // HTTP version
    char query_string[256];     // Query parameters
    char *body;                 // Request body
    int body_length;            // Body length
    char headers[32][128];      // Request headers
    int header_count;           // Header count
    char *raw_request;          // Raw request data
    int raw_length;             // Raw length
    const char *client_ip;      // Client IP
    uint16_t client_port;       // Client port
} http_request_t;
```

#### Response Formatting
```c
typedef struct {
    http_status_t status;       // HTTP status code
    char *body;                 // Response body
    int body_length;            // Body length
    http_content_type_t content_type; // Content type
    char headers[32][128];      // Response headers
    int header_count;           // Header count
    bool keep_alive;            // Keep-alive flag
    bool cache_enabled;         // Cache control
    int cache_max_age;          // Cache max age
    char *etag;                 // ETag header
    time_t last_modified;       // Last-Modified timestamp
} http_response_t;
```

### MIME Types

```c
static const struct {
    const char *extension;
    const char *mime_type;
} mime_types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {NULL, NULL}
};
```

### Usage Example

```c
void api_handler(http_request_t *request, http_response_t *response)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "timestamp", esp_timer_get_time());
    
    char *json_str = cJSON_PrintUnformatted(root);
    
    response->status = HTTP_STATUS_OK;
    response->body = json_str;
    response->body_length = strlen(json_str);
    strcpy(response->content_type_str, "application/json");
    
    cJSON_Delete(root);
}

// Register endpoint
http_endpoint_t api_endpoint = {
    .uri = "/api/status",
    .method = HTTP_METHOD_GET,
    .handler = api_handler
};
http_server_register_endpoint(&api_endpoint);
```

---

## Project Structure

```
Build ESP32 Smail GUI OS/
├── main/
│   ├── scheduler.c          # Main scheduler entry point
│   ├── scheduler.h
│   ├── main.c               # Main application entry
│   ├── small_os.c           # OS core functions
│   ├── web_server.c         # Web server integration
│   ├── gpio_manager.c       # GPIO management
│   └── ...
├── scheduler/
│   ├── task_scheduler.c     # Full task scheduler implementation
│   └── task_scheduler.h     # Scheduler header
├── bootloader/
│   ├── bootloader.c         # Bootloader code
│   └── bootloader.h
├── gpio/
│   ├── gpio_hal.c           # GPIO HAL implementation
│   └── gpio_hal.h           # GPIO HAL header
├── interrupt/
│   ├── interrupt_handler.c  # Interrupt handler
│   └── interrupt_handler.h  # Interrupt header
├── http/
│   ├── http_server.c        # HTTP server
│   └── http_server.h        # HTTP header
├── sim/
│   └── sim_runner.c         # Simulation runner
├── components/              # ESP-IDF components
├── Makefile                 # Build configuration
└── IMPLEMENTATION_GUIDE.md  # This file
```

---

## Memory Allocation Strategies

### Stack Allocation

Each task receives a dedicated stack:

```c
// Allocate stack for task
task->stack_ptr = (uint32_t*)malloc(stack_size);
if (!task->stack_ptr) {
    return ESP_ERR_NO_MEM;
}
task->stack_top = (uint32_t)task->stack_ptr + stack_size;

// Initialize stack frame
cpu_context_t *ctx = (cpu_context_t*)(task->stack_top - sizeof(cpu_context_t));
memset(ctx, 0, sizeof(cpu_context_t));
ctx->pc = (uint32_t)entry;
ctx->sp = (uint32_t)ctx;
ctx->ps = 0x40020;
```

### Heap Management

For dynamic allocations, use ESP-IDF heap functions:

```c
void *ptr = malloc(size);           // Standard malloc
void *ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT);  // 8-bit accessible
void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM); // External RAM
```

### Memory Pools

Fixed-size memory pools for frequently allocated objects:

```c
#define POOL_SIZE 64
static uint8_t pool_buffer[POOL_SIZE * sizeof(task_cb_t)] __attribute__((aligned(4)));
static bool pool_used[POOL_SIZE] = {false};

void *pool_alloc(void)
{
    for (int i = 0; i < POOL_SIZE; i++) {
        if (!pool_used[i]) {
            pool_used[i] = true;
            return &pool_buffer[i * sizeof(task_cb_t)];
        }
    }
    return NULL;
}

void pool_free(void *ptr)
{
    if (ptr >= (void*)pool_buffer && ptr < (void*)(pool_buffer + POOL_SIZE * sizeof(task_cb_t))) {
        int index = ((uint8_t*)ptr - pool_buffer) / sizeof(task_cb_t);
        pool_used[index] = false;
    }
}
```

### Memory Protection

Configure memory protection regions:

```c
esp_cpu_configure_region_protection();

// SRAM0 - Data RAM
REGION_IRAM0: 0x40000000 - 0x40020000 (128KB)
REGION_IRAM1: 0x40020000 - 0x40060000 (256KB) - Restricted

// SRAM1 - Instruction RAM  
REGION_IROM0: 0x40060000 - 0x40080000 (128KB) - Restricted

// SRAM2 - Mixed
REGION_SRAM2: 0x3FFB0000 - 0x3FFC0000 (64KB) - Read/write
```

---

## Build System

### Makefile Configuration

```makefile
PROJECT_NAME := esp32_small_os

# Compiler settings
CC := $(IDF_PATH)/components/espidf/make/component_wrapper.mk
CFLAGS := -Wall -Werror -Wno-unused-variable
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fstrict-aliasing -Wno-unused-but-set-variable

# Include paths
INCLUDES := -I$(PROJECT_PATH)/main
INCLUDES += -I$(PROJECT_PATH)/scheduler
INCLUDES += -I$(PROJECT_PATH)/bootloader
INCLUDES += -I$(PROJECT_PATH)/gpio
INCLUDES += -I$(PROJECT_PATH)/interrupt
INCLUDES += -I$(PROJECT_PATH)/http

# Source files
SRCS := $(wildcard main/*.c)
SRCS += $(wildcard scheduler/*.c)
SRCS += $(wildcard gpio/*.c)
SRCS += $(wildcard interrupt/*.c)
SRCS += $(wildcard http/*.c)

# Build target
build:
	$(CC) $(CFLAGS) $(INCLUDES) -c $(SRCS)
	$(AR) rcs lib$(PROJECT_NAME).a *.o

clean:
	rm -f *.o *.a
```

### PlatformIO Configuration

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf

build_flags = 
    -DSCHEDULER_MAX_TASKS=16
    -DSCHEDULER_TIMER_PERIOD_MS=1
    -I${PROJECT_DIR}/scheduler
    -I${PROJECT_DIR}/bootloader
    -I${PROJECT_DIR}/gpio
    -I${PROJECT_DIR}/interrupt
    -I${PROJECT_DIR}/http

lib_deps = 
    esp32/ESP32@^2.0.0
```

### Build Targets

```bash
# Build for ESP32
make build

# Build for simulation
gcc -DSIMULATION -o sim_runner sim/sim_runner.c

# Flash to device
make flash SERIAL_PORT=/dev/ttyUSB0
```

---

## Simulation Mode

### Overview

The simulation mode allows testing the Small OS on a PC without hardware. Key differences:

| Feature | Hardware | Simulation |
|---------|----------|------------|
| GPIO | Real pins | Simulated state |
| Timer | Hardware timer | Software timer |
| Network | WiFi | Loopback socket |
| Memory | 520KB RAM | Host memory |

### Simulation Runner

The `sim/sim_runner.c` provides:

1. **Timer Simulation**: Uses `setitimer` or `clock_nanosleep`
2. **GPIO Emulation**: Maintains pin state in arrays
3. **Network Loopback**: TCP sockets on localhost

### Running Simulation

```bash
# Compile simulation
gcc -DSIMULATION -o sim_runner sim/sim_runner.c scheduler/*.c

# Run simulation
./sim_runner
```

### Simulation-Specific Code

```c
#ifdef SIMULATION
    #include <sys/time.h>
    #include <signal.h>
    
    static void timer_handler(int sig) {
        scheduler_tick++;
        scheduler_time++;
    }
    
    void setup_timer(void) {
        struct sigaction sa;
        sa.sa_handler = timer_handler;
        sigemptyset(&sa.sa_flags);
        sigaction(SIGALRM, &sa, NULL);
        
        struct itimerval timer;
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = 1000;  // 1ms
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 1000;
        setitimer(ITIMER_REAL, &timer, NULL);
    }
#endif
```

### Debugging in Simulation

Use GDB or LLDB:

```bash
gdb ./sim_runner
(gdb) break scheduler_run_once
(gdb) run
(gdb) print *current_task
(gdb) continue
```