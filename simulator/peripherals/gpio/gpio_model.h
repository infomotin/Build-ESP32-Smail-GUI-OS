#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <functional>
#include <map>

// GPIO pin count and ranges
#define ESP32_GPIO_COUNT 40
#define ESP32_GPIO_MIN 0
#define ESP32_GPIO_MAX 39
#define ESP32_INPUT_ONLY_START 34
#define ESP32_INPUT_ONLY_END 39

// GPIO modes
enum class GpioMode {
    DISABLED = 0,      // Pin disabled
    INPUT = 1,         // Digital input
    OUTPUT = 2,        // Digital output
    INPUT_PULLUP = 3,  // Input with pull-up
    INPUT_PULLDOWN = 4, // Input with pull-down
    OUTPUT_OD = 5,      // Open-drain output
    ADC = 6,           // ADC input (GPIO32-39 only)
    DAC = 7,           // DAC output (GPIO25,26 only)
    PWM = 8,           // PWM/LEDC output
    I2C = 9,           // I2C mode
    SPI = 10,          // SPI mode
    UART = 11          // UART mode
};

// GPIO interrupt types
enum class GpioInterruptType {
    DISABLED = 0,
    POSEDGE = 1,       // Rising edge
    NEGEDGE = 2,       // Falling edge
    ANYEDGE = 3,       // Both edges
    LOW_LEVEL = 4,      // Low level trigger
    HIGH_LEVEL = 5      // High level trigger
};

// GPIO pin state
struct GpioPinState {
    GpioMode mode;
    bool output_level;
    bool input_level;
    bool pull_up;
    bool pull_down;
    bool open_drain;
    GpioInterruptType interrupt_type;
    bool interrupt_enabled;
    bool interrupt_pending;
    uint32_t last_change_time;
    
    // ADC specific
    bool adc_enabled;
    uint16_t adc_value;
    uint8_t adc_attenuation;
    float adc_voltage;
    
    // DAC specific
    bool dac_enabled;
    uint8_t dac_value;
    float dac_voltage;
    
    // PWM specific
    bool pwm_enabled;
    uint32_t pwm_frequency;
    uint32_t pwm_duty;
    uint32_t pwm_channel;
    float pwm_voltage;
    
    // Connection to virtual components
    bool connected_to_component;
    std::string component_name;
    
    GpioPinState() : mode(GpioMode::DISABLED), output_level(false), input_level(false),
                    pull_up(false), pull_down(false), open_drain(false),
                    interrupt_type(GpioInterruptType::DISABLED), interrupt_enabled(false),
                    interrupt_pending(false), last_change_time(0), adc_enabled(false),
                    adc_value(0), adc_attenuation(0), adc_voltage(0.0f), dac_enabled(false),
                    dac_value(0), dac_voltage(0.0f), pwm_enabled(false), pwm_frequency(0),
                    pwm_duty(0), pwm_channel(0), pwm_voltage(0.0f), connected_to_component(false) {}
};

// GPIO configuration structure
struct GpioConfig {
    GpioMode mode;
    GpioInterruptType interrupt_type;
    bool pull_up;
    bool pull_down;
    bool open_drain;
    
    // ADC configuration
    uint8_t adc_attenuation;
    
    // DAC configuration
    uint8_t dac_value;
    
    // PWM configuration
    uint32_t pwm_frequency;
    uint32_t pwm_duty;
    uint32_t pwm_channel;
    
    GpioConfig() : mode(GpioMode::DISABLED), interrupt_type(GpioInterruptType::DISABLED),
                  pull_up(false), pull_down(false), open_drain(false), adc_attenuation(0),
                  dac_value(0), pwm_frequency(1000), pwm_duty(50), pwm_channel(0) {}
};

// GPIO event types
enum class GpioEventType {
    PIN_MODE_CHANGED,
    PIN_LEVEL_CHANGED,
    INTERRUPT_TRIGGERED,
    ADC_READING,
    DAC_UPDATED,
    PWM_UPDATED
};

// GPIO event structure
struct GpioEvent {
    uint8_t pin;
    GpioEventType event_type;
    uint64_t timestamp;
    uint32_t data;
    
    GpioEvent(uint8_t p, GpioEventType type, uint64_t ts, uint32_t d = 0)
        : pin(p), event_type(type), timestamp(ts), data(d) {}
};

// GPIO callback function type
using GpioCallback = std::function<void(const GpioEvent&)>;

// LEDC PWM configuration
struct LedcConfig {
    uint32_t frequency;     // PWM frequency in Hz (1 Hz to 40 MHz)
    uint32_t duty;          // Duty cycle (0 to 100%)
    uint32_t resolution;    // Bit resolution (1 to 20 bits)
    uint8_t channel;        // PWM channel (0 to 15)
    uint8_t timer;          // Timer (0 to 3)
    uint8_t speed_mode;     // Speed mode (0 or 1)
    
    LedcConfig() : frequency(1000), duty(50), resolution(8), channel(0), timer(0), speed_mode(0) {}
};

// ADC configuration
struct AdcConfig {
    uint8_t attenuation;    // 0=0dB, 1=2.5dB, 2=6dB, 3=11dB
    uint8_t bit_width;      // 9, 10, 11, or 12 bits
    uint32_t v_ref;         // Reference voltage in mV (default 3300mV)
    bool enable_noise;      // Enable noise simulation
    
    AdcConfig() : attenuation(0), bit_width(12), v_ref(3300), enable_noise(true) {}
};

// DAC configuration
struct DacConfig {
    uint8_t bit_width;      // 8 bits fixed
    uint32_t v_ref;         // Reference voltage in mV (default 3300mV)
    bool enable_noise;      // Enable noise simulation
    
    DacConfig() : bit_width(8), v_ref(3300), enable_noise(false) {}
};

// GPIO Model class
class GpioModel {
public:
    GpioModel();
    ~GpioModel();
    
    // Pin configuration
    bool configure_pin(uint8_t pin, const GpioConfig& config);
    bool set_pin_mode(uint8_t pin, GpioMode mode);
    GpioMode get_pin_mode(uint8_t pin) const;
    
    // Digital I/O
    bool set_output_level(uint8_t pin, bool level);
    bool get_input_level(uint8_t pin) const;
    bool get_output_level(uint8_t pin) const;
    
    // Pull-up/pull-down
    bool set_pull_up(uint8_t pin, bool enable);
    bool set_pull_down(uint8_t pin, bool enable);
    bool get_pull_up(uint8_t pin) const;
    bool get_pull_down(uint8_t pin) const;
    
    // Open-drain
    bool set_open_drain(uint8_t pin, bool enable);
    bool get_open_drain(uint8_t pin) const;
    
    // Interrupts
    bool configure_interrupt(uint8_t pin, GpioInterruptType type, bool enable);
    bool enable_interrupt(uint8_t pin);
    bool disable_interrupt(uint8_t pin);
    bool get_interrupt_pending(uint8_t pin) const;
    void clear_interrupt_pending(uint8_t pin);
    std::vector<uint8_t> get_pending_interrupts() const;
    
    // ADC operations (GPIO32-39 only)
    bool enable_adc(uint8_t pin, const AdcConfig& config);
    bool disable_adc(uint8_t pin);
    uint16_t read_adc(uint8_t pin);
    float read_adc_voltage(uint8_t pin);
    void set_adc_input_voltage(uint8_t pin, float voltage);
    
    // DAC operations (GPIO25,26 only)
    bool enable_dac(uint8_t pin, const DacConfig& config);
    bool disable_dac(uint8_t pin);
    bool write_dac(uint8_t pin, uint8_t value);
    float get_dac_voltage(uint8_t pin) const;
    
    // PWM/LEDC operations
    bool enable_pwm(uint8_t pin, const LedcConfig& config);
    bool disable_pwm(uint8_t pin);
    bool set_pwm_duty(uint8_t pin, uint32_t duty);
    bool set_pwm_frequency(uint8_t pin, uint32_t frequency);
    uint32_t get_pwm_duty(uint8_t pin) const;
    uint32_t get_pwm_frequency(uint8_t pin) const;
    float get_pwm_voltage(uint8_t pin) const;
    
    // Virtual component connections
    bool connect_component(uint8_t pin, const std::string& component_name);
    bool disconnect_component(uint8_t pin);
    bool is_component_connected(uint8_t pin) const;
    const std::string& get_connected_component(uint8_t pin) const;
    
    // Event handling
    void register_callback(GpioCallback callback);
    void unregister_callback();
    
    // State queries
    const GpioPinState& get_pin_state(uint8_t pin) const;
    std::vector<GpioPinState> get_all_pin_states() const;
    bool is_input_only_pin(uint8_t pin) const;
    bool is_valid_pin(uint8_t pin) const;
    
    // Simulation control
    void update(uint64_t current_time);
    void reset();
    
    // Statistics
    struct Statistics {
        uint64_t pin_mode_changes;
        uint64_t level_changes;
        uint64_t interrupts_triggered;
        uint64_t adc_readings;
        uint64_t dac_updates;
        uint64_t pwm_updates;
        uint64_t component_connections;
    };
    const Statistics& get_statistics() const { return stats; }
    void reset_statistics() { stats = {}; }
    
    // Debug functions
    void dump_pin_state(uint8_t pin) const;
    void dump_all_pins() const;

private:
    // Pin states
    std::vector<GpioPinState> pins;
    
    // Configuration
    AdcConfig adc_config;
    DacConfig dac_config;
    std::map<uint8_t, LedcConfig> pwm_configs;
    
    // Event handling
    GpioCallback event_callback;
    std::vector<GpioEvent> event_queue;
    
    // Simulation state
    uint64_t current_time;
    bool simulation_running;
    
    // Statistics
    Statistics stats;
    
    // Internal methods
    void update_pin_input(uint8_t pin);
    void check_interrupts(uint8_t pin);
    void trigger_interrupt(uint8_t pin);
    void emit_event(const GpioEvent& event);
    void update_pwm_voltage(uint8_t pin);
    void update_adc_voltage(uint8_t pin);
    void update_dac_voltage(uint8_t pin);
    float calculate_dac_voltage(uint8_t value) const;
    float calculate_adc_voltage(uint16_t adc_value) const;
    uint16_t calculate_adc_raw_value(float voltage) const;
    float add_adc_noise(float voltage) const;
    float add_dac_noise(float voltage) const;
    bool is_valid_mode_for_pin(uint8_t pin, GpioMode mode) const;
    
    // PWM/LEDC simulation
    struct PwmState {
        uint64_t last_update;
        uint32_t period_counter;
        bool pwm_output;
    };
    std::map<uint8_t, PwmState> pwm_states;
};
