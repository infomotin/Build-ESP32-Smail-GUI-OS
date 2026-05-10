#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===================== System Configuration ===================== */
#define SMALL_OS_VERSION           "1.0.0"
#define SMALL_OS_MAX_PINS          40
#define SMALL_OS_MAX_ACTIVITIES    16
#define SMALL_OS_MAX_SCHEDULES     16
#define SMALL_OS_MAX_WEB_RESOURCES 50
#define SMALL_OS_GPIO_PINS_COUNT   26  /* GPIO0-GPIO25 usable */
#define SMALL_OS_WIFI_SSID_MAX     32
#define SMALL_OS_WIFI_PASS_MAX     64
#define SMALL_OS_DEVICE_NAME_MAX   32
#define SMALL_OS_LOG_BUFFER_SIZE   4096
#define SMALL_OS_TASK_STACK_SIZE   4096
#define SMALL_OS_TICK_RATE_HZ      100

/* ===================== Event Types ===================== */
typedef enum {
    EVENT_NONE = 0,
    EVENT_TIMER,
    EVENT_ACTIVITY,
    EVENT_API,
    EVENT_GPIO_CHANGE,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_MAX
} event_type_t;

/* ===================== Event Structure ===================== */
typedef struct event {
    event_type_t type;
    int id;
    void *data;
    struct event *next;
} event_t;

/* ===================== GPIO Pin Configuration ===================== */
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_OUTPUT_OPEN_DRAIN,
    GPIO_MODE_INPUT_OUTPUT,
    GPIO_MODE_MAX
} gpio_mode_t;

typedef struct {
    int pin;
    gpio_mode_t mode;
    int value;
    char label[32];
    bool in_use;
} gpio_pin_t;

/* ===================== PWM Configuration ===================== */
typedef struct {
    int pin;
    uint32_t frequency;
    uint32_t duty;      /* 0-255 for 8-bit resolution */
    bool enabled;
} pwm_config_t;

/* ===================== ADC Configuration ===================== */
typedef struct {
    int channel;
    int raw_value;
    float voltage;
    char label[32];
} adc_channel_t;

/* ===================== Activity Action Types ===================== */
typedef enum {
    ACTION_GPIO_SET = 0,
    ACTION_GPIO_TOGGLE,
    ACTION_GPIO_PWM,
    ACTION_DELAY,
    ACTION_CONDITION,
    ACTION_HTTP_REQUEST,
    ACTION_MAX
} action_type_t;

/* ===================== Activity Action ===================== */
typedef struct action {
    action_type_t type;
    int param1;
    int param2;
    int param3;
    char name[32];
    struct action *next;
} action_t;

/* ===================== Activity Definition ===================== */
typedef struct {
    int id;
    char name[32];
    char description[64];
    action_t *actions;
    bool enabled;
    uint32_t trigger_count;
} activity_t;

/* ===================== Schedule Entry ===================== */
typedef struct {
    int id;
    int activity_id;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool enabled;
    bool repeat;
    char name[32];
} schedule_t;

/* ===================== System Status ===================== */
typedef struct {
    uint32_t uptime_seconds;
    uint32_t free_heap;
    uint32_t min_free_heap;
    uint32_t total_heap;
    float cpu_usage;
    uint32_t wifi_status;
    uint32_t gpio_active_count;
    uint32_t activity_count;
    uint32_t schedule_count;
    uint32_t event_queue_size;
    uint32_t event_queue_max;
    char device_name[SMALL_OS_DEVICE_NAME_MAX];
    char version[16];
    char ip_address[16];
    char mac_address[18];
} system_status_t;

/* ===================== Module Return Codes ===================== */
typedef enum {
    SM_OS_OK = 0,
    SM_OS_ERROR = -1,
    SM_OS_BUSY = -2,
    SM_OS_NOT_FOUND = -3,
    SM_OS_NO_MEM = -4,
    SM_OS_INVALID_PARAM = -5,
    SM_OS_TIMEOUT = -6
} small_os_result_t;