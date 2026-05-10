#include "gpio_hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/rmt.h"
#include <string.h>
#include <stdlib.h>

#define TAG "GPIO_HAL"
#define MAX_GPIO_PINS 40
#define PWM_FREQ_BASE 5000  // 5kHz base frequency
#define ADC_VREF 1100  // mV
#define ADC_MAX_VALUE 4095  // 12-bit ADC

// GPIO pin state tracking
static gpio_pin_config_t pin_configs[MAX_GPIO_PINS];
static gpio_pin_state_t pin_states[MAX_GPIO_PINS];
static void (*gpio_handlers[MAX_GPIO_PINS])(uint8_t pin, void *arg);
static void *gpio_handler_args[MAX_GPIO_PINS];
static bool gpio_initialized = false;
static gpio_stats_t gpio_stats;
static uint32_t gpio_init_time = 0;

// PWM channel allocation
static ledc_channel_t pwm_channels[LEDC_CHANNEL_MAX];
static bool pwm_channel_used[LEDC_CHANNEL_MAX] = {false};
static uint8_t pin_to_pwm_channel[MAX_GPIO_PINS];

// ADC channel allocation
static adc1_channel_t adc_channels[ADC1_CHANNEL_MAX];
static bool adc_channel_used[ADC1_CHANNEL_MAX] = {false};
static uint8_t pin_to_adc_channel[MAX_GPIO_PINS];

// I2C port allocation
static i2c_port_t i2c_ports[I2C_NUM_MAX];
static bool i2c_port_used[I2C_NUM_MAX] = {false};
static uint8_t pin_to_i2c_port[MAX_GPIO_PINS];

// SPI host allocation
static spi_host_device_t spi_hosts[SPI_HOST_MAX];
static bool spi_host_used[SPI_HOST_MAX] = {false};
static uint8_t pin_to_spi_host[MAX_GPIO_PINS];

// UART port allocation
static uart_port_t uart_ports[UART_NUM_MAX];
static bool uart_port_used[UART_NUM_MAX] = {false};
static uint8_t pin_to_uart_port[MAX_GPIO_PINS];

// RMT channel allocation
static rmt_channel_t rmt_channels[RMT_CHANNEL_MAX];
static bool rmt_channel_used[RMT_CHANNEL_MAX] = {false};
static uint8_t pin_to_rmt_channel[MAX_GPIO_PINS];

// ESP32 GPIO capabilities
static const uint32_t gpio_capabilities[MAX_GPIO_PINS] = {
    // GPIO 0-39 capabilities (simplified)
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 0-9
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 10-19
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // 20-29
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F  // 30-39
};

esp_err_t gpio_hal_init(void)
{
    if (gpio_initialized) {
        return ESP_OK;
    }
    
    // Initialize statistics
    memset(&gpio_stats, 0, sizeof(gpio_stats_t));
    gpio_init_time = esp_timer_get_time() / 1000;
    
    // Clear pin configurations and states
    memset(pin_configs, 0, sizeof(pin_configs));
    memset(pin_states, 0, sizeof(pin_states));
    memset(gpio_handlers, 0, sizeof(gpio_handlers));
    memset(gpio_handler_args, 0, sizeof(gpio_handler_args));
    memset(pwm_channel_used, 0, sizeof(pwm_channel_used));
    memset(adc_channel_used, 0, sizeof(adc_channel_used));
    memset(i2c_port_used, 0, sizeof(i2c_port_used));
    memset(spi_host_used, 0, sizeof(spi_host_used));
    memset(uart_port_used, 0, sizeof(uart_port_used));
    memset(rmt_channel_used, 0, sizeof(rmt_channel_used));
    memset(pin_to_pwm_channel, 0xFF, sizeof(pin_to_pwm_channel));
    memset(pin_to_adc_channel, 0xFF, sizeof(pin_to_adc_channel));
    memset(pin_to_i2c_port, 0xFF, sizeof(pin_to_i2c_port));
    memset(pin_to_spi_host, 0xFF, sizeof(pin_to_spi_host));
    memset(pin_to_uart_port, 0xFF, sizeof(pin_to_uart_port));
    memset(pin_to_rmt_channel, 0xFF, sizeof(pin_to_rmt_channel));
    
    // Initialize GPIO subsystem
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize LEDC PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ_BASE,
        .clk_cfg = LEDC_AUTO_CLK
    };
    err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize ADC
    err = adc1_config_width(ADC_WIDTH_BIT_12);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC width: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize DAC
    err = dac_output_enable(DAC_CHANNEL_1);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "DAC channel 1 not available: %s", esp_err_to_name(err));
    }
    err = dac_output_enable(DAC_CHANNEL_2);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "DAC channel 2 not available: %s", esp_err_to_name(err));
    }
    
    gpio_initialized = true;
    ESP_LOGI(TAG, "GPIO HAL initialized successfully");
    return ESP_OK;
}

esp_err_t gpio_hal_configure_pin(const gpio_pin_config_t *config)
{
    if (!config || config->pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if pin supports the requested mode
    if (!gpio_hal_is_mode_supported(config->pin, config->mode)) {
        ESP_LOGE(TAG, "Pin %d does not support mode %d", config->pin, config->mode);
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = ESP_OK;
    
    // Configure based on mode
    switch (config->mode) {
        case GPIO_MODE_INPUT:
        case GPIO_MODE_INPUT_PULLUP:
        case GPIO_MODE_INPUT_PULLDOWN: {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << config->pin),
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = (config->mode == GPIO_MODE_INPUT_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
                .pull_down_en = (config->mode == GPIO_MODE_INPUT_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
                .intr_type = config->intr_type
            };
            err = gpio_config(&io_conf);
            break;
        }
        
        case GPIO_MODE_OUTPUT:
        case GPIO_MODE_OUTPUT_OD: {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << config->pin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            err = gpio_config(&io_conf);
            break;
        }
        
        case GPIO_MODE_PWM: {
            // Find free PWM channel
            ledc_channel_t channel;
            for (int i = 0; i < LEDC_CHANNEL_MAX; i++) {
                if (!pwm_channel_used[i]) {
                    channel = i;
                    pwm_channel_used[i] = true;
                    break;
                }
            }
            
            if (pwm_channel_used[channel]) {
                ESP_LOGE(TAG, "No free PWM channels available");
                return ESP_ERR_NO_MEM;
            }
            
            ledc_channel_config_t ledc_channel = {
                .gpio_num = config->pin,
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .channel = channel,
                .intr_type = LEDC_INTR_DISABLE,
                .timer_sel = LEDC_TIMER_0,
                .duty = 0,
                .hpoint = 0
            };
            err = ledc_channel_config(&ledc_channel);
            if (err == ESP_OK) {
                pin_to_pwm_channel[config->pin] = channel;
                // Set initial PWM configuration
                err = gpio_hal_set_pwm(config->pin, &config->config.pwm);
            }
            break;
        }
        
        case GPIO_MODE_ADC: {
            // Find free ADC channel
            adc1_channel_t channel;
            for (int i = 0; i < ADC1_CHANNEL_MAX; i++) {
                if (!adc_channel_used[i]) {
                    channel = i;
                    adc_channel_used[i] = true;
                    break;
                }
            }
            
            if (adc_channel_used[channel]) {
                ESP_LOGE(TAG, "No free ADC channels available");
                return ESP_ERR_NO_MEM;
            }
            
            err = adc1_config_channel_atten(channel, config->config.adc.attenuation);
            if (err == ESP_OK) {
                pin_to_adc_channel[config->pin] = channel;
            }
            break;
        }
        
        case GPIO_MODE_DAC: {
            err = gpio_hal_set_dac(config->pin, &config->config.dac);
            break;
        }
        
        case GPIO_MODE_I2C: {
            err = gpio_hal_set_i2c(config->config.i2c.sda_io_num, config->config.i2c.scl_io_num, &config->config.i2c);
            break;
        }
        
        case GPIO_MODE_SPI: {
            err = gpio_hal_set_spi(config->config.spi.mosi_io_num, config->config.spi.miso_io_num,
                               config->config.spi.sclk_io_num, config->config.spi.cs_io_num, &config->config.spi);
            break;
        }
        
        case GPIO_MODE_UART: {
            err = gpio_hal_set_uart(config->config.uart.tx_io_num, config->config.uart.rx_io_num, &config->config.uart);
            break;
        }
        
        case GPIO_MODE_RMT: {
            err = gpio_hal_set_rmt(config->pin, &config->config.rmt);
            break;
        }
        
        default:
            return ESP_ERR_INVALID_ARG;
    }
    
    if (err == ESP_OK) {
        // Store configuration
        pin_configs[config->pin] = *config;
        
        // Update pin state
        pin_states[config->pin].pin = config->pin;
        pin_states[config->pin].mode = config->mode;
        pin_states[config->pin].level = gpio_get_level(config->pin);
        pin_states[config->pin].last_change_time = esp_timer_get_time() / 1000;
        pin_states[config->pin].interrupt_count = 0;
        
        ESP_LOGD(TAG, "Configured GPIO %d as mode %d", config->pin, config->mode);
    } else {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", config->pin, esp_err_to_name(err));
        gpio_stats.error_count++;
    }
    
    return err;
}

esp_err_t gpio_hal_set_level(uint8_t pin, bool level)
{
    if (pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (pin_configs[pin].mode != GPIO_MODE_OUTPUT && 
        pin_configs[pin].mode != GPIO_MODE_OUTPUT_OD) {
        ESP_LOGW(TAG, "Pin %d not configured as output", pin);
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = gpio_set_level(pin, level ? 1 : 0);
    if (err == ESP_OK) {
        pin_states[pin].level = level;
        pin_states[pin].last_change_time = esp_timer_get_time() / 1000;
        gpio_stats.total_writes++;
    } else {
        gpio_stats.error_count++;
    }
    
    return err;
}

bool gpio_hal_get_level(uint8_t pin)
{
    if (pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return false;
    }
    
    bool level = gpio_get_level(pin) ? true : false;
    pin_states[pin].level = level;
    gpio_stats.total_reads++;
    
    return level;
}

esp_err_t gpio_hal_set_pwm(uint8_t pin, const pwm_config_t *config)
{
    if (pin >= MAX_GPIO_PINS || !config || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (pin_configs[pin].mode != GPIO_MODE_PWM) {
        ESP_LOGW(TAG, "Pin %d not configured as PWM", pin);
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t channel = pin_to_pwm_channel[pin];
    if (channel >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = ESP_OK;
    
    // Update frequency if different
    if (config->frequency != PWM_FREQ_BASE) {
        err = ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, config->frequency);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set PWM frequency: %s", esp_err_to_name(err));
            return err;
        }
    }
    
    // Update duty cycle
    uint32_t duty = (config->duty_cycle * 255) / 100;
    err = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    if (err == ESP_OK) {
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    }
    
    if (err == ESP_OK) {
        // Store configuration
        pin_configs[pin].config.pwm = *config;
        gpio_stats.total_pwm_updates++;
        
        ESP_LOGD(TAG, "Set PWM on GPIO %d: freq=%dHz, duty=%d%%", 
                 pin, config->frequency, config->duty_cycle);
    } else {
        gpio_stats.error_count++;
    }
    
    return err;
}

esp_err_t gpio_hal_set_pwm_duty(uint8_t pin, uint32_t duty_cycle)
{
    if (pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (pin_configs[pin].mode != GPIO_MODE_PWM) {
        ESP_LOGW(TAG, "Pin %d not configured as PWM", pin);
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t channel = pin_to_pwm_channel[pin];
    if (channel >= LEDC_CHANNEL_MAX) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint32_t duty = (duty_cycle * 255) / 100;
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    if (err == ESP_OK) {
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
    }
    
    if (err == ESP_OK) {
        pin_configs[pin].config.pwm.duty_cycle = duty_cycle;
        gpio_stats.total_pwm_updates++;
    } else {
        gpio_stats.error_count++;
    }
    
    return err;
}

esp_err_t gpio_hal_set_adc(uint8_t pin, const adc_config_t *config)
{
    if (pin >= MAX_GPIO_PINS || !config || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (pin_configs[pin].mode != GPIO_MODE_ADC) {
        ESP_LOGW(TAG, "Pin %d not configured as ADC", pin);
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t channel = pin_to_adc_channel[pin];
    if (channel >= ADC1_CHANNEL_MAX) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = adc1_config_channel_atten(channel, config->attenuation);
    if (err == ESP_OK) {
        // Store configuration
        pin_configs[pin].config.adc = *config;
        ESP_LOGD(TAG, "Configured ADC on GPIO %d: channel=%d, atten=%d", 
                 pin, channel, config->attenuation);
    } else {
        gpio_stats.error_count++;
    }
    
    return err;
}

int gpio_hal_read_adc(uint8_t pin)
{
    if (pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return -1;
    }
    
    if (pin_configs[pin].mode != GPIO_MODE_ADC) {
        ESP_LOGW(TAG, "Pin %d not configured as ADC", pin);
        return -1;
    }
    
    uint8_t channel = pin_to_adc_channel[pin];
    if (channel >= ADC1_CHANNEL_MAX) {
        return -1;
    }
    
    int raw_value = adc1_get_raw(channel);
    if (raw_value >= 0) {
        gpio_stats.total_adc_reads++;
        ESP_LOGD(TAG, "ADC read GPIO %d: %d", pin, raw_value);
    } else {
        gpio_stats.error_count++;
    }
    
    return raw_value;
}

esp_err_t gpio_hal_read_adc_voltage(uint8_t pin, float *voltage)
{
    if (!voltage) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int raw_value = gpio_hal_read_adc(pin);
    if (raw_value < 0) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Convert to voltage (simplified)
    uint8_t channel = pin_to_adc_channel[pin];
    adc_atten_t atten = pin_configs[pin].config.adc.attenuation;
    
    float attenuation_factor = 1.0f;
    switch (atten) {
        case ADC_ATTEN_DB_0:   attenuation_factor = 1.0f; break;
        case ADC_ATTEN_DB_2_5: attenuation_factor = 1.33f; break;
        case ADC_ATTEN_DB_6:   attenuation_factor = 2.0f; break;
        case ADC_ATTEN_DB_11:  attenuation_factor = 3.6f; break;
        default: attenuation_factor = 1.0f; break;
    }
    
    *voltage = (raw_value * ADC_VREF * attenuation_factor) / ADC_MAX_VALUE;
    
    return ESP_OK;
}

// GPIO interrupt handler
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    
    // Update statistics
    gpio_stats.total_interrupts++;
    
    // Update pin state
    if (gpio_num < MAX_GPIO_PINS) {
        pin_states[gpio_num].level = gpio_get_level(gpio_num);
        pin_states[gpio_num].last_change_time = esp_timer_get_time() / 1000;
        pin_states[gpio_num].interrupt_count++;
        
        // Call user handler if registered
        if (gpio_handlers[gpio_num]) {
            gpio_handlers[gpio_num](gpio_num, gpio_handler_args[gpio_num]);
        }
    }
}

esp_err_t gpio_hal_enable_interrupt(uint8_t pin, gpio_intr_type_t intr_type, void (*handler)(uint8_t pin, void *arg), void *arg)
{
    if (pin >= MAX_GPIO_PINS || !handler || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Store handler
    gpio_handlers[pin] = handler;
    gpio_handler_args[pin] = arg;
    
    // Configure interrupt
    esp_err_t err = gpio_set_intr_type(pin, intr_type);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set interrupt type: %s", esp_err_to_name(err));
        return err;
    }
    
    err = gpio_isr_handler_add(pin, gpio_isr_handler, (void*)pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler: %s", esp_err_to_name(err));
        return err;
    }
    
    err = gpio_intr_enable(pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable interrupt: %s", esp_err_to_name(err));
        return err;
    }
    
    // Update configuration
    pin_configs[pin].intr_type = intr_type;
    
    ESP_LOGI(TAG, "Enabled interrupt on GPIO %d, type %d", pin, intr_type);
    return ESP_OK;
}

esp_err_t gpio_hal_disable_interrupt(uint8_t pin)
{
    if (pin >= MAX_GPIO_PINS || !gpio_initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = gpio_intr_disable(pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable interrupt: %s", esp_err_to_name(err));
        return err;
    }
    
    err = gpio_isr_handler_remove(pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove ISR handler: %s", esp_err_to_name(err));
        return err;
    }
    
    // Clear handler
    gpio_handlers[pin] = NULL;
    gpio_handler_args[pin] = NULL;
    pin_configs[pin].intr_type = GPIO_INTR_DISABLE;
    
    ESP_LOGI(TAG, "Disabled interrupt on GPIO %d", pin);
    return ESP_OK;
}

bool gpio_hal_is_pin_valid(uint8_t pin)
{
    return pin < MAX_GPIO_PINS;
}

bool gpio_hal_is_mode_supported(uint8_t pin, gpio_mode_t mode)
{
    if (pin >= MAX_GPIO_PINS) {
        return false;
    }
    
    // ESP32 GPIO capability check (simplified)
    uint32_t caps = gpio_capabilities[pin];
    
    switch (mode) {
        case GPIO_MODE_INPUT:
        return (caps & 0x01) != 0;
        case GPIO_MODE_OUTPUT:
            return (caps & 0x02) != 0;
        case GPIO_MODE_PWM:
            return (caps & 0x04) != 0;
        case GPIO_MODE_ADC:
            return (caps & 0x08) != 0;
        case GPIO_MODE_DAC:
            return (caps & 0x10) != 0;
        case GPIO_MODE_I2C:
            return (caps & 0x20) != 0;
        case GPIO_MODE_SPI:
            return (caps & 0x40) != 0;
        case GPIO_MODE_UART:
            return (caps & 0x80) != 0;
        default:
            return false;
    }
}

esp_err_t gpio_hal_get_statistics(gpio_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *stats = gpio_stats;
    return ESP_OK;
}

void gpio_hal_print_statistics(void)
{
    uint32_t current_time = esp_timer_get_time() / 1000;
    uint32_t uptime = current_time - gpio_init_time;
    
    ESP_LOGI(TAG, "=== GPIO HAL Statistics ===");
    ESP_LOGI(TAG, "Uptime: %d seconds", uptime);
    ESP_LOGI(TAG, "Total interrupts: %d", gpio_stats.total_interrupts);
    ESP_LOGI(TAG, "Total reads: %d", gpio_stats.total_reads);
    ESP_LOGI(TAG, "Total writes: %d", gpio_stats.total_writes);
    ESP_LOGI(TAG, "Total PWM updates: %d", gpio_stats.total_pwm_updates);
    ESP_LOGI(TAG, "Total ADC reads: %d", gpio_stats.total_adc_reads);
    ESP_LOGI(TAG, "Error count: %d", gpio_stats.error_count);
    ESP_LOGI(TAG, "===========================");
}

esp_err_t gpio_hal_self_test(void)
{
    ESP_LOGI(TAG, "Starting GPIO HAL self-test...");
    
    esp_err_t result = ESP_OK;
    
    // Test basic GPIO configuration
    gpio_pin_config_t test_config = {
        .pin = 2,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up = false,
        .pull_down = false
    };
    
    esp_err_t err = gpio_hal_configure_pin(&test_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO configuration test failed");
        result = ESP_FAIL;
    }
    
    // Test GPIO level setting
    err = gpio_hal_set_level(2, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO level setting test failed");
        result = ESP_FAIL;
    }
    
    // Test GPIO level reading
    bool level = gpio_hal_get_level(2);
    if (!level) {
        ESP_LOGE(TAG, "GPIO level reading test failed");
        result = ESP_FAIL;
    }
    
    // Test PWM configuration
    pwm_config_t pwm_config = {
        .frequency = 1000,
        .duty_cycle = 50,
        .resolution = 8,
        .timer_num = LEDC_TIMER_0,
        .speed_mode = LEDC_LOW_SPEED_MODE
    };
    
    gpio_pin_config_t pwm_test_config = {
        .pin = 4,
        .mode = GPIO_MODE_PWM,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up = false,
        .pull_down = false,
        .config.pwm = pwm_config
    };
    
    err = gpio_hal_configure_pin(&pwm_test_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PWM configuration test failed");
        result = ESP_FAIL;
    }
    
    // Test PWM duty cycle setting
    err = gpio_hal_set_pwm_duty(4, 75);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PWM duty cycle setting test failed");
        result = ESP_FAIL;
    }
    
    // Test ADC configuration
    adc_config_t adc_config = {
        .channel = ADC1_CHANNEL_0,
        .attenuation = ADC_ATTEN_DB_11,
        .bit_width = ADC_WIDTH_BIT_12,
        .unit = ADC_UNIT_1,
        .use_vref = false
    };
    
    gpio_pin_config_t adc_test_config = {
        .pin = 36,
        .mode = GPIO_MODE_ADC,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up = false,
        .pull_down = false,
        .config.adc = adc_config
    };
    
    err = gpio_hal_configure_pin(&adc_test_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC configuration test failed");
        result = ESP_FAIL;
    }
    
    // Test ADC reading
    int adc_value = gpio_hal_read_adc(36);
    if (adc_value < 0) {
        ESP_LOGE(TAG, "ADC reading test failed");
        result = ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "GPIO HAL self-test %s", result == ESP_OK ? "PASSED" : "FAILED");
    return result;
}
