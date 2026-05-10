#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/rmt.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

// GPIO modes
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_OUTPUT_OD,
    GPIO_MODE_PWM,
    GPIO_MODE_ADC,
    GPIO_MODE_DAC,
    GPIO_MODE_I2C,
    GPIO_MODE_SPI,
    GPIO_MODE_UART,
    GPIO_MODE_RMT
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
    uint32_t duty_cycle;   // 0-100%
    uint8_t resolution;    // bits (1, 2, 4, 8, 10, 12)
    ledc_timer_t timer_num;
    ledc_mode_t speed_mode;
} pwm_config_t;

// ADC configuration
typedef struct {
    adc1_channel_t channel;
    adc2_channel_t channel2;
    adc_atten_t attenuation;
    adc_bits_width_t bit_width;
    adc_unit_t unit;
    bool use_vref;
} adc_config_t;

// DAC configuration
typedef struct {
    dac_channel_t channel;
    uint8_t value;        // 0-255
    bool enable;
} dac_config_t;

// I2C configuration
typedef struct {
    i2c_port_t port;
    uint32_t sda_io_num;
    uint32_t scl_io_num;
    uint32_t clk_speed;
    i2c_mode_t mode;
    bool pullup_enable;
} i2c_config_t;

// SPI configuration
typedef struct {
    spi_host_device_t host;
    int mosi_io_num;
    int miso_io_num;
    int sclk_io_num;
    int cs_io_num;
    int max_transfer_sz;
    uint32_t clock_speed_hz;
    spi_mode_t mode;
    bool cs_ena_pretrans;
    bool cs_ena_posttrans;
} spi_config_t;

// UART configuration
typedef struct {
    uart_port_t uart_num;
    int tx_io_num;
    int rx_io_num;
    int baud_rate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;
    bool rx_flow_control_thresh;
} uart_config_t;

// RMT configuration
typedef struct {
    rmt_channel_t channel;
    rmt_mode_t mode;
    gpio_num_t gpio_num;
    uint32_t clk_div;
    uint32_t mem_block_num;
    uint32_t flags;
} rmt_config_t;

// GPIO pin configuration
typedef struct {
    uint8_t pin;
    gpio_mode_t mode;
    gpio_intr_type_t intr_type;
    bool pull_up;
    bool pull_down;
    bool drive_strength;
    union {
        pwm_config_t pwm;
        adc_config_t adc;
        dac_config_t dac;
        i2c_config_t i2c;
        spi_config_t spi;
        uart_config_t uart;
        rmt_config_t rmt;
    } config;
} gpio_pin_config_t;

// GPIO pin state
typedef struct {
    uint8_t pin;
    gpio_mode_t mode;
    bool level;
    uint32_t last_change_time;
    uint32_t interrupt_count;
    void (*handler)(uint8_t pin, void *arg);
    void *handler_arg;
} gpio_pin_state_t;

// GPIO statistics
typedef struct {
    uint32_t total_interrupts;
    uint32_t total_reads;
    uint32_t total_writes;
    uint32_t total_pwm_updates;
    uint32_t total_adc_reads;
    uint32_t error_count;
    uint32_t last_reset_time;
} gpio_stats_t;

// Function declarations
esp_err_t gpio_hal_init(void);
esp_err_t gpio_hal_deinit(void);
esp_err_t gpio_hal_configure_pin(const gpio_pin_config_t *config);
esp_err_t gpio_hal_reconfigure_pin(uint8_t pin, const gpio_pin_config_t *config);
esp_err_t gpio_hal_set_level(uint8_t pin, bool level);
bool gpio_hal_get_level(uint8_t pin);
esp_err_t gpio_hal_set_direction(uint8_t pin, gpio_mode_t mode);
esp_err_t gpio_hal_set_pull_mode(uint8_t pin, bool pull_up, bool pull_down);
esp_err_t gpio_hal_set_drive_strength(uint8_t pin, bool strength);

// PWM functions
esp_err_t gpio_hal_set_pwm(uint8_t pin, const pwm_config_t *config);
esp_err_t gpio_hal_set_pwm_duty(uint8_t pin, uint32_t duty_cycle);
esp_err_t gpio_hal_set_pwm_frequency(uint8_t pin, uint32_t frequency);
esp_err_t gpio_hal_get_pwm_config(uint8_t pin, pwm_config_t *config);
esp_err_t gpio_hal_stop_pwm(uint8_t pin);

// ADC functions
esp_err_t gpio_hal_set_adc(uint8_t pin, const adc_config_t *config);
int gpio_hal_read_adc(uint8_t pin);
esp_err_t gpio_hal_read_adc_raw(uint8_t pin, int *raw_value);
esp_err_t gpio_hal_read_adc_voltage(uint8_t pin, float *voltage);
esp_err_t gpio_hal_calibrate_adc(uint8_t pin);
esp_err_t gpio_hal_get_adc_config(uint8_t pin, adc_config_t *config);

// DAC functions
esp_err_t gpio_hal_set_dac(uint8_t pin, const dac_config_t *config);
esp_err_t gpio_hal_set_dac_value(uint8_t pin, uint8_t value);
esp_err_t gpio_hal_enable_dac(uint8_t pin);
esp_err_t gpio_hal_disable_dac(uint8_t pin);

// I2C functions
esp_err_t gpio_hal_set_i2c(uint8_t sda_pin, uint8_t scl_pin, const i2c_config_t *config);
esp_err_t gpio_hal_i2c_write(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr, const uint8_t *data, size_t len);
esp_err_t gpio_hal_i2c_read(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr, uint8_t *data, size_t len);
esp_err_t gpio_hal_i2c_scan(uint8_t sda_pin, uint8_t scl_pin, uint8_t *devices, size_t max_devices);

// SPI functions
esp_err_t gpio_hal_set_spi(uint8_t mosi_pin, uint8_t miso_pin, uint8_t sclk_pin, uint8_t cs_pin, const spi_config_t *config);
esp_err_t gpio_hal_spi_write(uint8_t mosi_pin, uint8_t miso_pin, uint8_t sclk_pin, uint8_t cs_pin, const uint8_t *data, size_t len);
esp_err_t gpio_hal_spi_read(uint8_t mosi_pin, uint8_t miso_pin, uint8_t sclk_pin, uint8_t cs_pin, uint8_t *data, size_t len);
esp_err_t gpio_hal_spi_transfer(uint8_t mosi_pin, uint8_t miso_pin, uint8_t sclk_pin, uint8_t cs_pin, const uint8_t *tx_data, uint8_t *rx_data, size_t len);

// UART functions
esp_err_t gpio_hal_set_uart(uint8_t tx_pin, uint8_t rx_pin, const uart_config_t *config);
esp_err_t gpio_hal_uart_write(uint8_t tx_pin, uint8_t rx_pin, const uint8_t *data, size_t len);
esp_err_t gpio_hal_uart_read(uint8_t tx_pin, uint8_t rx_pin, uint8_t *data, size_t len, uint32_t timeout_ms);
esp_err_t gpio_hal_uart_available(uint8_t tx_pin, uint8_t rx_pin, size_t *available);

// RMT functions
esp_err_t gpio_hal_set_rmt(uint8_t pin, const rmt_config_t *config);
esp_err_t gpio_hal_rmt_write(uint8_t pin, const uint8_t *data, size_t len);
esp_err_t gpio_hal_rmt_read(uint8_t pin, uint8_t *data, size_t *len, uint32_t timeout_ms);

// Interrupt functions
esp_err_t gpio_hal_enable_interrupt(uint8_t pin, gpio_intr_type_t intr_type, void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t gpio_hal_disable_interrupt(uint8_t pin);
esp_err_t gpio_hal_set_interrupt_type(uint8_t pin, gpio_intr_type_t intr_type);
gpio_intr_type_t gpio_hal_get_interrupt_type(uint8_t pin);
esp_err_t gpio_hal_register_isr_handler(uint8_t pin, void (*handler)(uint8_t pin, void *arg), void *arg);
esp_err_t gpio_hal_unregister_isr_handler(uint8_t pin);

// Utility functions
esp_err_t gpio_hal_get_pin_config(uint8_t pin, gpio_pin_config_t *config);
esp_err_t gpio_hal_get_pin_state(uint8_t pin, gpio_pin_state_t *state);
esp_err_t gpio_hal_reset_pin(uint8_t pin);
esp_err_t gpio_hal_reset_all_pins(void);
bool gpio_hal_is_pin_valid(uint8_t pin);
bool gpio_hal_is_mode_supported(uint8_t pin, gpio_mode_t mode);
const char* gpio_hal_get_mode_name(gpio_mode_t mode);
const char* gpio_hal_get_intr_type_name(gpio_intr_type_t intr_type);

// Statistics functions
esp_err_t gpio_hal_get_statistics(gpio_stats_t *stats);
esp_err_t gpio_hal_reset_statistics(void);
void gpio_hal_print_statistics(void);

// Batch operations
esp_err_t gpio_hal_set_multiple_levels(const uint8_t *pins, const bool *levels, size_t count);
esp_err_t gpio_hal_get_multiple_levels(const uint8_t *pins, bool *levels, size_t count);
esp_err_t gpio_hal_configure_multiple_pins(const gpio_pin_config_t *configs, size_t count);

// Power management
esp_err_t gpio_hal_enable_sleep_pin(uint8_t pin);
esp_err_t gpio_hal_disable_sleep_pin(uint8_t pin);
esp_err_t gpio_hal_set_wakeup_pin(uint8_t pin, gpio_intr_type_t intr_type);

// Hardware-specific functions
esp_err_t gpio_hal_get_esp32_capabilities(uint32_t *capabilities);
esp_err_t gpio_hal_get_pin_capabilities(uint8_t pin, uint32_t *capabilities);
esp_err_t gpio_hal_get_pin_matrix(uint8_t pin, uint32_t *input_signal, uint32_t *output_signal);

// Debug functions
void gpio_hal_dump_pin_config(uint8_t pin);
void gpio_hal_dump_all_configs(void);
void gpio_hal_test_pin(uint8_t pin);
esp_err_t gpio_hal_self_test(void);
