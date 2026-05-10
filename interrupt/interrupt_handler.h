#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "gpio_hal.h"

// Event types
#define EVENT_TYPE_GPIO        0x01
#define EVENT_TYPE_TIMER       0x02
#define EVENT_TYPE_NETWORK     0x03
#define EVENT_TYPE_HTTP        0x04
#define EVENT_TYPE_SYSTEM      0x05
#define EVENT_TYPE_USER        0x06
#define EVENT_TYPE_ADC         0x07
#define EVENT_TYPE_PWM         0x08
#define EVENT_TYPE_I2C        0x09
#define EVENT_TYPE_SPI        0x0A
#define EVENT_TYPE_UART       0x0B

// Event priorities
#define EVENT_PRIORITY_CRITICAL  0
#define EVENT_PRIORITY_HIGH     1
#define EVENT_PRIORITY_NORMAL   2
#define EVENT_PRIORITY_LOW      3
#define EVENT_PRIORITY_IDLE     4

// Debounce configuration
#define DEBOUNCE_TIME_MS        50
#define DEBOUNCE_SAMPLES        3
#define DEBOUNCE_STABLE_COUNT    2

// Interrupt configuration
#define MAX_INTERRUPT_SOURCES    32
#define MAX_PENDING_EVENTS     64
#define ISR_QUEUE_SIZE         32

// Interrupt source structure
typedef struct {
    uint8_t pin;
    gpio_intr_type_t type;
    void (*handler)(uint8_t pin, void *arg);
    void *arg;
    uint32_t last_interrupt_time;
    uint32_t interrupt_count;
    bool debouncing;
    bool enabled;
    uint8_t debounce_state;
    uint8_t stable_count;
    uint32_t priority;
} interrupt_source_t;

// Event structure
typedef struct {
    uint32_t type;
    uint32_t source;
    void *data;
    uint32_t timestamp;
    uint32_t priority;
    uint8_t retry_count;
    bool from_isr;
} interrupt_event_t;

// Interrupt statistics
typedef struct {
    uint32_t total_interrupts;
    uint32_t total_events;
    uint32_t missed_events;
    uint32_t debounce_rejections;
    uint32_t max_queue_depth;
    uint32_t avg_processing_time;
    uint32_t max_processing_time;
    uint32_t last_reset_time;
    uint32_t interrupts_per_source[MAX_INTERRUPT_SOURCES];
} interrupt_stats_t;

// Interrupt manager configuration
typedef struct {
    uint32_t max_sources;
    uint32_t max_events;
    uint32_t debounce_time_ms;
    bool enable_statistics;
    bool enable_priority_queue;
    bool enable_debounce;
    uint32_t isr_stack_size;
    uint32_t task_priority;
} interrupt_config_t;

// Function declarations
esp_err_t interrupt_init(const interrupt_config_t *config);
esp_err_t interrupt_deinit(void);
esp_err_t interrupt_register_source(uint8_t pin, gpio_intr_type_t type, 
                                  void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t interrupt_unregister_source(uint8_t pin);
esp_err_t interrupt_enable_source(uint8_t pin);
esp_err_t interrupt_disable_source(uint8_t pin);
esp_err_t interrupt_set_priority(uint8_t pin, uint32_t priority);
esp_err_t interrupt_post_event(const interrupt_event_t *event);
esp_err_t interrupt_post_event_from_isr(const interrupt_event_t *event);
esp_err_t interrupt_get_event(interrupt_event_t *event, uint32_t timeout_ms);
esp_err_t interrupt_peek_event(interrupt_event_t *event);
void interrupt_process_events(void);
bool interrupt_is_debouncing(uint8_t pin);
void interrupt_set_debounce(uint8_t pin, bool debouncing);
esp_err_t interrupt_configure_debounce(uint8_t pin, uint32_t debounce_ms, uint8_t samples);
esp_err_t interrupt_get_source_config(uint8_t pin, interrupt_source_t *config);
esp_err_t interrupt_get_statistics(interrupt_stats_t *stats);
void interrupt_print_statistics(void);
esp_err_t interrupt_reset_statistics(void);
void interrupt_dump_sources(void);
esp_err_t interrupt_self_test(void);

// ISR-specific functions
static void IRAM_ATTR gpio_isr_handler(void *arg);
static void IRAM_ATTR timer_isr_handler(void *arg);
static void IRAM_ATTR network_isr_handler(void *arg);
static bool IRAM_ATTR debounce_interrupt(uint8_t pin, bool level);
static void IRAM_ATTR queue_event_from_isr(const interrupt_event_t *event);

// Internal functions
static interrupt_source_t* find_source_by_pin(uint8_t pin);
static interrupt_source_t* find_free_source_slot(void);
static esp_err_t add_to_pending_queue(const interrupt_event_t *event);
static interrupt_event_t* get_next_event(void);
static void update_statistics(const interrupt_event_t *event, uint32_t processing_time);
static void process_debounce(void);
static void handle_gpio_interrupt(uint8_t pin);
static void handle_timer_event(uint32_t timer_id);
static void handle_network_event(uint32_t event_type);

// Hardware abstraction
static esp_err_t configure_gpio_interrupt(uint8_t pin, gpio_intr_type_t type);
static esp_err_t setup_timer_interrupt(uint32_t timer_id, uint32_t interval_ms);
static esp_err_t setup_network_interrupts(void);
