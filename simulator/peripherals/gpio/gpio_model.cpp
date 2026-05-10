#include "gpio_model.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <random>

GpioModel::GpioModel() : current_time(0), simulation_running(false) {
    pins.resize(ESP32_GPIO_COUNT);
    
    // Initialize default ADC and DAC configurations
    adc_config = AdcConfig();
    dac_config = DacConfig();
    
    // Mark input-only pins
    for (uint8_t pin = ESP32_INPUT_ONLY_START; pin <= ESP32_INPUT_ONLY_END; pin++) {
        pins[pin].mode = GpioMode::DISABLED;
    }
    
    stats = {};
}

GpioModel::~GpioModel() {
    // Cleanup handled by STL containers
}

bool GpioModel::configure_pin(uint8_t pin, const GpioConfig& config) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    if (!is_valid_mode_for_pin(pin, config.mode)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    GpioMode old_mode = state.mode;
    
    // Apply configuration
    state.mode = config.mode;
    state.pull_up = config.pull_up;
    state.pull_down = config.pull_down;
    state.open_drain = config.open_drain;
    state.interrupt_type = config.interrupt_type;
    state.interrupt_enabled = false;
    state.interrupt_pending = false;
    
    // Configure ADC if requested
    if (config.mode == GpioMode::ADC) {
        state.adc_enabled = true;
        state.adc_attenuation = config.adc_attenuation;
        update_adc_voltage(pin);
    } else {
        state.adc_enabled = false;
    }
    
    // Configure DAC if requested
    if (config.mode == GpioMode::DAC) {
        state.dac_enabled = true;
        state.dac_value = config.dac_value;
        update_dac_voltage(pin);
    } else {
        state.dac_enabled = false;
    }
    
    // Configure PWM if requested
    if (config.mode == GpioMode::PWM) {
        state.pwm_enabled = true;
        state.pwm_frequency = config.pwm_frequency;
        state.pwm_duty = config.pwm_duty;
        state.pwm_channel = config.pwm_channel;
        pwm_configs[pin] = LedcConfig();
        pwm_configs[pin].frequency = config.pwm_frequency;
        pwm_configs[pin].duty = config.pwm_duty;
        pwm_configs[pin].channel = config.pwm_channel;
        update_pwm_voltage(pin);
    } else {
        state.pwm_enabled = false;
        pwm_configs.erase(pin);
    }
    
    // Reset output level for input modes
    if (config.mode == GpioMode::INPUT || config.mode == GpioMode::INPUT_PULLUP || 
        config.mode == GpioMode::INPUT_PULLDOWN || config.mode == GpioMode::ADC) {
        state.output_level = false;
    }
    
    stats.pin_mode_changes++;
    emit_event(GpioEvent(pin, GpioEventType::PIN_MODE_CHANGED, current_time, static_cast<uint32_t>(old_mode)));
    
    return true;
}

bool GpioModel::set_pin_mode(uint8_t pin, GpioMode mode) {
    GpioConfig config;
    config.mode = mode;
    return configure_pin(pin, config);
}

GpioMode GpioModel::get_pin_mode(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return GpioMode::DISABLED;
    }
    return pins[pin].mode;
}

bool GpioModel::set_output_level(uint8_t pin, bool level) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    
    if (state.mode == GpioMode::DISABLED || state.mode == GpioMode::INPUT || 
        state.mode == GpioMode::INPUT_PULLUP || state.mode == GpioMode::INPUT_PULLDOWN || 
        state.mode == GpioMode::ADC) {
        return false; // Cannot set output on input pins
    }
    
    if (is_input_only_pin(pin)) {
        return false; // Input-only pins
    }
    
    bool old_level = state.output_level;
    state.output_level = level;
    state.last_change_time = current_time;
    
    // Update pin input level (for open-drain and component connections)
    update_pin_input(pin);
    
    if (old_level != level) {
        stats.level_changes++;
        emit_event(GpioEvent(pin, GpioEventType::PIN_LEVEL_CHANGED, current_time, level));
        check_interrupts(pin);
    }
    
    return true;
}

bool GpioModel::get_input_level(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].input_level;
}

bool GpioModel::get_output_level(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].output_level;
}

bool GpioModel::set_pull_up(uint8_t pin, bool enable) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    state.pull_up = enable;
    
    if (enable && state.mode == GpioMode::INPUT) {
        set_pin_mode(pin, GpioMode::INPUT_PULLUP);
    } else if (!enable && state.mode == GpioMode::INPUT_PULLUP) {
        set_pin_mode(pin, GpioMode::INPUT);
    }
    
    update_pin_input(pin);
    return true;
}

bool GpioModel::set_pull_down(uint8_t pin, bool enable) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    state.pull_down = enable;
    
    if (enable && state.mode == GpioMode::INPUT) {
        set_pin_mode(pin, GpioMode::INPUT_PULLDOWN);
    } else if (!enable && state.mode == GpioMode::INPUT_PULLDOWN) {
        set_pin_mode(pin, GpioMode::INPUT);
    }
    
    update_pin_input(pin);
    return true;
}

bool GpioModel::get_pull_up(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].pull_up;
}

bool GpioModel::get_pull_down(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].pull_down;
}

bool GpioModel::set_open_drain(uint8_t pin, bool enable) {
    if (!is_valid_pin(pin) || is_input_only_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    state.open_drain = enable;
    
    if (enable && state.mode == GpioMode::OUTPUT) {
        set_pin_mode(pin, GpioMode::OUTPUT_OD);
    } else if (!enable && state.mode == GpioMode::OUTPUT_OD) {
        set_pin_mode(pin, GpioMode::OUTPUT);
    }
    
    update_pin_input(pin);
    return true;
}

bool GpioModel::get_open_drain(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].open_drain;
}

bool GpioModel::configure_interrupt(uint8_t pin, GpioInterruptType type, bool enable) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    state.interrupt_type = type;
    state.interrupt_enabled = enable;
    state.interrupt_pending = false;
    
    return true;
}

bool GpioModel::enable_interrupt(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].interrupt_enabled = true;
    return true;
}

bool GpioModel::disable_interrupt(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].interrupt_enabled = false;
    pins[pin].interrupt_pending = false;
    return true;
}

bool GpioModel::get_interrupt_pending(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    return pins[pin].interrupt_pending;
}

void GpioModel::clear_interrupt_pending(uint8_t pin) {
    if (is_valid_pin(pin)) {
        pins[pin].interrupt_pending = false;
    }
}

std::vector<uint8_t> GpioModel::get_pending_interrupts() const {
    std::vector<uint8_t> pending;
    
    for (uint8_t pin = 0; pin < ESP32_GPIO_COUNT; pin++) {
        if (pins[pin].interrupt_pending) {
            pending.push_back(pin);
        }
    }
    
    return pending;
}

bool GpioModel::enable_adc(uint8_t pin, const AdcConfig& config) {
    if (!is_valid_pin(pin) || pin < ESP32_INPUT_ONLY_START) {
        return false; // ADC only on GPIO32-39
    }
    
    GpioPinState& state = pins[pin];
    state.adc_enabled = true;
    state.adc_attenuation = config.attenuation;
    adc_config = config;
    
    update_adc_voltage(pin);
    return true;
}

bool GpioModel::disable_adc(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].adc_enabled = false;
    return true;
}

uint16_t GpioModel::read_adc(uint8_t pin) {
    if (!is_valid_pin(pin) || !pins[pin].adc_enabled) {
        return 0;
    }
    
    update_adc_voltage(pin);
    stats.adc_readings++;
    emit_event(GpioEvent(pin, GpioEventType::ADC_READING, current_time, pins[pin].adc_value));
    
    return pins[pin].adc_value;
}

float GpioModel::read_adc_voltage(uint8_t pin) {
    if (!is_valid_pin(pin) || !pins[pin].adc_enabled) {
        return 0.0f;
    }
    
    update_adc_voltage(pin);
    return pins[pin].adc_voltage;
}

void GpioModel::set_adc_input_voltage(uint8_t pin, float voltage) {
    if (!is_valid_pin(pin) || !pins[pin].adc_enabled) {
        return;
    }
    
    // Clamp voltage to valid range
    voltage = std::max(0.0f, std::min(voltage, 3.3f));
    
    // Apply attenuation
    float max_voltage = 1.1f; // Default max voltage
    switch (pins[pin].adc_attenuation) {
        case 0: max_voltage = 1.1f; break;  // 0dB
        case 1: max_voltage = 1.5f; break;  // 2.5dB
        case 2: max_voltage = 2.2f; break;  // 6dB
        case 3: max_voltage = 3.3f; break;  // 11dB
    }
    
    float normalized_voltage = std::min(voltage, max_voltage);
    pins[pin].adc_voltage = normalized_voltage;
    pins[pin].adc_value = calculate_adc_raw_value(normalized_voltage);
}

bool GpioModel::enable_dac(uint8_t pin, const DacConfig& config) {
    if (!is_valid_pin(pin) || (pin != 25 && pin != 26)) {
        return false; // DAC only on GPIO25,26
    }
    
    GpioPinState& state = pins[pin];
    state.dac_enabled = true;
    dac_config = config;
    
    update_dac_voltage(pin);
    return true;
}

bool GpioModel::disable_dac(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].dac_enabled = false;
    return true;
}

bool GpioModel::write_dac(uint8_t pin, uint8_t value) {
    if (!is_valid_pin(pin) || !pins[pin].dac_enabled) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    uint8_t old_value = state.dac_value;
    state.dac_value = value;
    
    update_dac_voltage(pin);
    
    if (old_value != value) {
        stats.dac_updates++;
        emit_event(GpioEvent(pin, GpioEventType::DAC_UPDATED, current_time, value));
    }
    
    return true;
}

float GpioModel::get_dac_voltage(uint8_t pin) const {
    if (!is_valid_pin(pin) || !pins[pin].dac_enabled) {
        return 0.0f;
    }
    
    return pins[pin].dac_voltage;
}

bool GpioModel::enable_pwm(uint8_t pin, const LedcConfig& config) {
    if (!is_valid_pin(pin) || is_input_only_pin(pin)) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    state.pwm_enabled = true;
    state.pwm_frequency = config.frequency;
    state.pwm_duty = config.duty;
    state.pwm_channel = config.channel;
    
    pwm_configs[pin] = config;
    update_pwm_voltage(pin);
    
    return true;
}

bool GpioModel::disable_pwm(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].pwm_enabled = false;
    pwm_configs.erase(pin);
    return true;
}

bool GpioModel::set_pwm_duty(uint8_t pin, uint32_t duty) {
    if (!is_valid_pin(pin) || !pins[pin].pwm_enabled) {
        return false;
    }
    
    GpioPinState& state = pins[pin];
    uint32_t old_duty = state.pwm_duty;
    state.pwm_duty = duty;
    
    if (pwm_configs.find(pin) != pwm_configs.end()) {
        pwm_configs[pin].duty = duty;
    }
    
    update_pwm_voltage(pin);
    
    if (old_duty != duty) {
        stats.pwm_updates++;
        emit_event(GpioEvent(pin, GpioEventType::PWM_UPDATED, current_time, duty));
    }
    
    return true;
}

bool GpioModel::set_pwm_frequency(uint8_t pin, uint32_t frequency) {
    if (!is_valid_pin(pin) || !pins[pin].pwm_enabled) {
        return false;
    }
    
    pins[pin].pwm_frequency = frequency;
    
    if (pwm_configs.find(pin) != pwm_configs.end()) {
        pwm_configs[pin].frequency = frequency;
    }
    
    update_pwm_voltage(pin);
    return true;
}

uint32_t GpioModel::get_pwm_duty(uint8_t pin) const {
    if (!is_valid_pin(pin) || !pins[pin].pwm_enabled) {
        return 0;
    }
    
    return pins[pin].pwm_duty;
}

uint32_t GpioModel::get_pwm_frequency(uint8_t pin) const {
    if (!is_valid_pin(pin) || !pins[pin].pwm_enabled) {
        return 0;
    }
    
    return pins[pin].pwm_frequency;
}

float GpioModel::get_pwm_voltage(uint8_t pin) const {
    if (!is_valid_pin(pin) || !pins[pin].pwm_enabled) {
        return 0.0f;
    }
    
    return pins[pin].pwm_voltage;
}

bool GpioModel::connect_component(uint8_t pin, const std::string& component_name) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].connected_to_component = true;
    pins[pin].component_name = component_name;
    stats.component_connections++;
    
    update_pin_input(pin);
    return true;
}

bool GpioModel::disconnect_component(uint8_t pin) {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    pins[pin].connected_to_component = false;
    pins[pin].component_name.clear();
    
    update_pin_input(pin);
    return true;
}

bool GpioModel::is_component_connected(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        return false;
    }
    
    return pins[pin].connected_to_component;
}

const std::string& GpioModel::get_connected_component(uint8_t pin) const {
    static std::string empty;
    
    if (!is_valid_pin(pin)) {
        return empty;
    }
    
    return pins[pin].component_name;
}

void GpioModel::register_callback(GpioCallback callback) {
    event_callback = callback;
}

void GpioModel::unregister_callback() {
    event_callback = nullptr;
}

const GpioPinState& GpioModel::get_pin_state(uint8_t pin) const {
    static GpioPinState empty_state;
    
    if (!is_valid_pin(pin)) {
        return empty_state;
    }
    
    return pins[pin];
}

std::vector<GpioPinState> GpioModel::get_all_pin_states() const {
    return pins;
}

bool GpioModel::is_input_only_pin(uint8_t pin) const {
    return pin >= ESP32_INPUT_ONLY_START && pin <= ESP32_INPUT_ONLY_END;
}

bool GpioModel::is_valid_pin(uint8_t pin) const {
    return pin <= ESP32_GPIO_MAX;
}

void GpioModel::update(uint64_t current_time) {
    this->current_time = current_time;
    
    // Update PWM outputs
    for (auto& config_pair : pwm_configs) {
        uint8_t pin = config_pair.first;
        if (pins[pin].pwm_enabled) {
            update_pwm_voltage(pin);
        }
    }
    
    // Process event queue
    if (event_callback) {
        for (const auto& event : event_queue) {
            event_callback(event);
        }
        event_queue.clear();
    }
}

void GpioModel::reset() {
    for (uint8_t pin = 0; pin < ESP32_GPIO_COUNT; pin++) {
        pins[pin] = GpioPinState();
    }
    
    pwm_configs.clear();
    event_queue.clear();
    current_time = 0;
    stats = {};
}

void GpioModel::dump_pin_state(uint8_t pin) const {
    if (!is_valid_pin(pin)) {
        std::cout << "Invalid pin: " << (int)pin << std::endl;
        return;
    }
    
    const GpioPinState& state = pins[pin];
    
    std::cout << "GPIO " << (int)pin << " State:\n";
    std::cout << "  Mode: " << static_cast<int>(state.mode) << "\n";
    std::cout << "  Output Level: " << (state.output_level ? "HIGH" : "LOW") << "\n";
    std::cout << "  Input Level: " << (state.input_level ? "HIGH" : "LOW") << "\n";
    std::cout << "  Pull-up: " << (state.pull_up ? "enabled" : "disabled") << "\n";
    std::cout << "  Pull-down: " << (state.pull_down ? "enabled" : "disabled") << "\n";
    std::cout << "  Open-drain: " << (state.open_drain ? "enabled" : "disabled") << "\n";
    std::cout << "  Interrupt: " << static_cast<int>(state.interrupt_type);
    std::cout << (state.interrupt_enabled ? " (enabled)" : " (disabled)") << "\n";
    std::cout << "  Connected to component: " << (state.connected_to_component ? state.component_name : "none") << "\n";
    
    if (state.adc_enabled) {
        std::cout << "  ADC: enabled, value=" << state.adc_value << ", voltage=" << std::fixed << std::setprecision(3) << state.adc_voltage << "V\n";
    }
    
    if (state.dac_enabled) {
        std::cout << "  DAC: enabled, value=" << (int)state.dac_value << ", voltage=" << std::fixed << std::setprecision(3) << state.dac_voltage << "V\n";
    }
    
    if (state.pwm_enabled) {
        std::cout << "  PWM: enabled, freq=" << state.pwm_frequency << "Hz, duty=" << state.pwm_duty << "%\n";
    }
}

void GpioModel::dump_all_pins() const {
    for (uint8_t pin = 0; pin < ESP32_GPIO_COUNT; pin++) {
        if (pins[pin].mode != GpioMode::DISABLED) {
            dump_pin_state(pin);
            std::cout << std::endl;
        }
    }
}

void GpioModel::update_pin_input(uint8_t pin) {
    GpioPinState& state = pins[pin];
    
    if (state.mode == GpioMode::DISABLED) {
        state.input_level = false;
        return;
    }
    
    bool new_input_level = false;
    
    // Check if connected to component
    if (state.connected_to_component) {
        // Component drives the pin level
        // This would be updated by the component itself
        return;
    }
    
    // Calculate input level based on configuration
    switch (state.mode) {
        case GpioMode::INPUT:
        case GpioMode::INPUT_PULLUP:
        case GpioMode::INPUT_PULLDOWN:
            new_input_level = state.pull_up; // Default to pull-up state
            break;
            
        case GpioMode::OUTPUT:
        case GpioMode::OUTPUT_OD:
            if (state.open_drain && state.output_level) {
                new_input_level = false; // Open-drain high is high-impedance
            } else {
                new_input_level = state.output_level;
            }
            break;
            
        case GpioMode::PWM:
            new_input_level = (state.pwm_voltage > 1.65f); // Threshold at 50% of 3.3V
            break;
            
        case GpioMode::DAC:
            new_input_level = (state.dac_voltage > 1.65f);
            break;
            
        case GpioMode::ADC:
            new_input_level = (state.adc_voltage > 1.65f);
            break;
            
        default:
            new_input_level = false;
            break;
    }
    
    if (new_input_level != state.input_level) {
        state.input_level = new_input_level;
        state.last_change_time = current_time;
        stats.level_changes++;
        emit_event(GpioEvent(pin, GpioEventType::PIN_LEVEL_CHANGED, current_time, new_input_level));
        check_interrupts(pin);
    }
}

void GpioModel::check_interrupts(uint8_t pin) {
    GpioPinState& state = pins[pin];
    
    if (!state.interrupt_enabled || state.interrupt_type == GpioInterruptType::DISABLED) {
        return;
    }
    
    bool should_trigger = false;
    
    switch (state.interrupt_type) {
        case GpioInterruptType::POSEDGE:
            should_trigger = (state.input_level && !state.output_level);
            break;
            
        case GpioInterruptType::NEGEDGE:
            should_trigger = (!state.input_level && state.output_level);
            break;
            
        case GpioInterruptType::ANYEDGE:
            should_trigger = (state.input_level != state.output_level);
            break;
            
        case GpioInterruptType::LOW_LEVEL:
            should_trigger = !state.input_level;
            break;
            
        case GpioInterruptType::HIGH_LEVEL:
            should_trigger = state.input_level;
            break;
            
        default:
            break;
    }
    
    if (should_trigger && !state.interrupt_pending) {
        trigger_interrupt(pin);
    }
}

void GpioModel::trigger_interrupt(uint8_t pin) {
    pins[pin].interrupt_pending = true;
    stats.interrupts_triggered++;
    emit_event(GpioEvent(pin, GpioEventType::INTERRUPT_TRIGGERED, current_time));
}

void GpioModel::emit_event(const GpioEvent& event) {
    if (event_callback) {
        event_callback(event);
    } else {
        event_queue.push_back(event);
    }
}

void GpioModel::update_pwm_voltage(uint8_t pin) {
    GpioPinState& state = pins[pin];
    
    if (!state.pwm_enabled) {
        state.pwm_voltage = 0.0f;
        return;
    }
    
    // Simple PWM simulation - use duty cycle directly
    float duty_fraction = state.pwm_duty / 100.0f;
    state.pwm_voltage = 3.3f * duty_fraction;
    
    // Add some noise for realism
    if (dac_config.enable_noise) {
        state.pwm_voltage = add_dac_noise(state.pwm_voltage);
    }
}

void GpioModel::update_adc_voltage(uint8_t pin) {
    GpioPinState& state = pins[pin];
    
    if (!state.adc_enabled) {
        state.adc_voltage = 0.0f;
        state.adc_value = 0;
        return;
    }
    
    // If no external voltage is set, simulate a default reading
    if (state.adc_voltage == 0.0f) {
        state.adc_voltage = 1.65f; // Mid-scale default
    }
    
    // Add noise if enabled
    if (adc_config.enable_noise) {
        state.adc_voltage = add_adc_noise(state.adc_voltage);
    }
    
    state.adc_value = calculate_adc_raw_value(state.adc_voltage);
}

void GpioModel::update_dac_voltage(uint8_t pin) {
    GpioPinState& state = pins[pin];
    
    if (!state.dac_enabled) {
        state.dac_voltage = 0.0f;
        return;
    }
    
    state.dac_voltage = calculate_dac_voltage(state.dac_value);
    
    // Add noise if enabled
    if (dac_config.enable_noise) {
        state.dac_voltage = add_dac_noise(state.dac_voltage);
    }
}

float GpioModel::calculate_dac_voltage(uint8_t value) const {
    return (value / 255.0f) * 3.3f;
}

float GpioModel::calculate_adc_voltage(uint16_t adc_value) const {
    float max_voltage = 1.1f; // Default
    
    switch (adc_config.bit_width) {
        case 12:  max_voltage = 3.3f; break;
        case 11:  max_voltage = 3.3f; break;
        case 10:  max_voltage = 3.3f; break;
        case 9:   max_voltage = 3.3f; break;
    }
    
    return (adc_value / static_cast<float>((1 << adc_config.bit_width) - 1)) * max_voltage;
}

uint16_t GpioModel::calculate_adc_raw_value(float voltage) const {
    float max_voltage = 1.1f; // Default
    
    switch (adc_config.bit_width) {
        case 12:  max_voltage = 3.3f; break;
        case 11:  max_voltage = 3.3f; break;
        case 10:  max_voltage = 3.3f; break;
        case 9:   max_voltage = 3.3f; break;
    }
    
    float normalized = std::max(0.0f, std::min(voltage / max_voltage, 1.0f));
    return static_cast<uint16_t>(normalized * ((1 << adc_config.bit_width) - 1));
}

float GpioModel::add_adc_noise(float voltage) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<float> noise(0.0f, 0.02f); // ±2 LSB noise
    
    float noise_value = noise(gen) * 3.3f / 4096.0f; // Convert to voltage
    return voltage + noise_value;
}

float GpioModel::add_dac_noise(float voltage) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<float> noise(0.0f, 0.01f); // ±1% noise
    
    float noise_value = noise(gen) * voltage * 0.01f;
    return voltage + noise_value;
}

bool GpioModel::is_valid_mode_for_pin(uint8_t pin, GpioMode mode) const {
    // Input-only pins can only be input modes
    if (is_input_only_pin(pin)) {
        return (mode == GpioMode::DISABLED || mode == GpioMode::INPUT || 
                mode == GpioMode::INPUT_PULLUP || mode == GpioMode::INPUT_PULLDOWN ||
                mode == GpioMode::ADC);
    }
    
    // DAC only on GPIO25,26
    if (mode == GpioMode::DAC && pin != 25 && pin != 26) {
        return false;
    }
    
    // ADC only on GPIO32-39
    if (mode == GpioMode::ADC && (pin < 32 || pin > 39)) {
        return false;
    }
    
    return true;
}
