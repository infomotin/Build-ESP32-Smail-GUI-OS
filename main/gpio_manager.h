/*
 * GPIO Manager Header
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/adc.h"

typedef void (*gpio_isr_t)(int pin, int value, void *arg);

/* Core API */
void gpio_manager_init(void);
esp_err_t gpio_configure_pin(int pin, gpio_mode_t mode);
esp_err_t gpio_set_level_safe(int pin, int level);
int gpio_get_level_safe(int pin);
esp_err_t gpio_set_pwm(int pin, uint32_t duty, uint32_t freq);
int adc_read_channel(adc1_channel_t channel);
float adc_read_voltage(adc1_channel_t channel);

/* Interrupts */
esp_err_t gpio_set_interrupt(int pin, gpio_int_type_t intr_type, gpio_isr_t isr_handler, void *arg);
esp_err_t gpio_toggle(int pin);

/* Label & Info */
esp_err_t gpio_set_label(int pin, const char *label);
typedef struct gpio_pin_state {
    int pin;
    int mode;
    int value;
    char label[32];
    bool in_use;
    bool has_pwm;
    uint32_t pwm_duty;
    uint32_t pwm_freq;
} gpio_pin_state_t;

const gpio_pin_state_t *gpio_get_all_pins(int *count);
const gpio_pin_state_t *gpio_get_pin(int pin);