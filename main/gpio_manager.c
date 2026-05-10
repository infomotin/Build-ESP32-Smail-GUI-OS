/*
 * GPIO Manager - Hardware Abstraction Layer
 * Supports: Digital Input/Output, PWM, ADC, Interrupts
 */
#include "small_os.h"
#include "gpio_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/timer.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "gpio_manager";

#define GPIO_MAX_PINS           26
#define GPIO_MAX_PWM_CHANNELS   8
#define GPIO_MAX_ADC_CHANNELS   8
#define GPIO_ISR_QUEUE_SIZE     32
#define GPIO_DEBOUNCE_MS        50

/* GPIO Pin State Tracking */
typedef struct {
    int pin;
    gpio_mode_t mode;
    int value;
    char label[32];
    bool in_use;
    bool has_pwm;
    uint32_t pwm_duty;
    uint32_t pwm_freq;
    pwm_config_t pwm_config;
    gpio_isr_t isr_handler;
    void *isr_arg;
    adc_channel_t adc_config;
} gpio_pin_state_t;

static gpio_pin_state_t gpio_pins[GPIO_MAX_PINS];
static QueueHandle_t gpio_isr_queue = NULL;
static ledc_channel_t pwm_channels[GPIO_MAX_PWM_CHANNELS];
static uint8_t pwm_channel_count = 0;

/* ISR handling task */
static TaskHandle_t gpio_isr_task_handle = NULL;

static void gpio_isr_task(void *arg)
{
    gpio_isr_event_t event;
    while (1) {
        if (xQueueReceive(gpio_isr_queue, &event, portMAX_DELAY) == pdTRUE) {
            int pin = event.gpio_num;
            if (pin >= 0 && pin < GPIO_MAX_PINS && gpio_pins[pin].isr_handler) {
                gpio_pins[pin].value = gpio_get_level(pin);
                gpio_pins[pin].isr_handler(pin, gpio_pins[pin].value, gpio_pins[pin].isr_arg);
            }
        }
    }
}

static int get_pwm_channel_for_pin(int pin)
{
    for (int i = 0; i < pwm_channel_count; i++) {
        if (pwm_channels[i].pin == pin) {
            return i;
        }
    }
    return -1;
}

void gpio_manager_init(void)
{
    memset(gpio_pins, 0, sizeof(gpio_pins));
    memset(pwm_channels, 0, sizeof(pwm_channels));
    pwm_channel_count = 0;

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    gpio_isr_queue = xQueueCreate(GPIO_ISR_QUEUE_SIZE, sizeof(gpio_isr_event_t));
    if (gpio_isr_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO ISR queue");
        return;
    }

    xTaskCreatePinnedToCore(gpio_isr_task, "gpio_isr", 2048, NULL, 5, &gpio_isr_task_handle, 0);

    /* Configure LEDC timer for PWM */
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(err));
    }

    /* ADC configuration */
    adc1_config_width(ADC_WIDTH_BIT_12);

    ESP_LOGI(TAG, "GPIO manager initialized");
}

esp_err_t gpio_configure_pin(int pin, gpio_mode_t mode)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode = (gpio_mode_t)mode,
        .pull_up_en = (mode == GPIO_MODE_INPUT_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (mode == GPIO_MODE_INPUT_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err == ESP_OK) {
        gpio_pins[pin].pin = pin;
        gpio_pins[pin].mode = mode;
        gpio_pins[pin].value = (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_OUTPUT_OPEN_DRAIN || mode == GPIO_MODE_INPUT_OUTPUT) ? 0 : gpio_get_level(pin);
        gpio_pins[pin].in_use = true;
        gpio_pins[pin].has_pwm = false;
        gpio_pins[pin].isr_handler = NULL;
        snprintf(gpio_pins[pin].label, sizeof(gpio_pins[pin].label), "GPIO%d", pin);
    }
    return err;
}

esp_err_t gpio_set_level_safe(int pin, int level)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gpio_pins[pin].in_use) {
        return ESP_ERR_INVALID_STATE;
    }

    /* If this pin has PWM, disable it first */
    if (gpio_pins[pin].has_pwm) {
        int ch = get_pwm_channel_for_pin(pin);
        if (ch >= 0) {
            ledc_stop(LEDC_HIGH_SPEED_MODE, ch, 0);
            gpio_pins[pin].has_pwm = false;
        }
    }

    gpio_set_level(pin, level ? 1 : 0);
    gpio_pins[pin].value = level ? 1 : 0;
    ESP_LOGD(TAG, "GPIO%d set to %d", pin, level);
    return ESP_OK;
}

int gpio_get_level_safe(int pin)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        return -1;
    }
    return gpio_get_level(pin);
}

esp_err_t gpio_set_pwm(int pin, uint32_t duty, uint32_t freq)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!gpio_pins[pin].in_use) {
        gpio_configure_pin(pin, GPIO_MODE_OUTPUT);
    }

    /* Ensure duty is within 13-bit range */
    if (duty > 8191) duty = 8191;

    int ch = get_pwm_channel_for_pin(pin);
    if (ch < 0) {
        if (pwm_channel_count >= GPIO_MAX_PWM_CHANNELS) {
            ESP_LOGE(TAG, "No free PWM channels");
            return ESP_ERR_NO_MEM;
        }
        ch = pwm_channel_count++;
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = duty,
        .hpoint = 0
    };

    esp_err_t err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) return err;

    err = ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, freq);
    if (err != ESP_OK) return err;

    err = ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch, duty);
    if (err == ESP_OK) {
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch);
    }

    pwm_channels[ch].pin = pin;
    gpio_pins[pin].has_pwm = true;
    gpio_pins[pin].pwm_duty = duty;
    gpio_pins[pin].pwm_freq = freq;

    return ESP_OK;
}

esp_err_t gpio_set_interrupt(int pin, gpio_int_type_t intr_type, gpio_isr_t isr_handler, void *arg)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_pins[pin].isr_handler = isr_handler;
    gpio_pins[pin].isr_arg = arg;

    gpio_set_intr_type(pin, intr_type);
    gpio_isr_handler_add(pin, gpio_intr_handler, (void *)&gpio_pins[pin]);

    ESP_LOGI(TAG, "ISR registered for GPIO%d", pin);
    return ESP_OK;
}

static void IRAM_ATTR gpio_intr_handler(void *arg)
{
    gpio_pin_state_t *pin_state = (gpio_pin_state_t *)arg;
    gpio_isr_event_t event = {
        .gpio_num = pin_state->pin
    };
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(gpio_isr_queue, &event, &higherPriorityTaskWoken);
    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

int adc_read_channel(adc1_channel_t channel)
{
    return adc1_get_raw(channel);
}

float adc_read_voltage(adc1_channel_t channel)
{
    int raw = adc1_get_raw(channel);
    /* ESP32 ADC: 0-4095 maps to 0-3.3V (approximately) */
    return (raw * 3.3f) / 4095.0f;
}

esp_err_t gpio_set_label(int pin, const char *label)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) return ESP_ERR_INVALID_ARG;
    if (!label) return ESP_ERR_INVALID_ARG;

    strncpy(gpio_pins[pin].label, label, sizeof(gpio_pins[pin].label) - 1);
    gpio_pins[pin].label[sizeof(gpio_pins[pin].label) - 1] = '\0';
    return ESP_OK;
}

const gpio_pin_state_t *gpio_get_all_pins(int *count)
{
    *count = GPIO_MAX_PINS;
    return gpio_pins;
}

const gpio_pin_state_t *gpio_get_pin(int pin)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) return NULL;
    return &gpio_pins[pin];
}

esp_err_t gpio_toggle(int pin)
{
    if (pin < 0 || pin >= GPIO_MAX_PINS) return ESP_ERR_INVALID_ARG;

    int current = gpio_get_level(pin);
    return gpio_set_level_safe(pin, !current);
}