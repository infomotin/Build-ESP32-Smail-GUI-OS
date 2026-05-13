/**
 * @file gpio_controller.cpp
 * @brief GPIO peripheral simulation implementation
 */

#include "peripherals/gpio_controller.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>

#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_ERR_INVALID_ARG
#define ESP_ERR_INVALID_ARG -1
#endif

namespace esp32sim {

GPIOController::GPIOController(MemoryModel* memory, QObject* parent)
    : QObject(parent), memory_(memory) {
    pins_.resize(GPIO_PIN_COUNT);
    pwm_states_.resize(GPIO_PIN_COUNT);
}

GPIOController::~GPIOController() = default;

bool GPIOController::initialize() {
    LOG_INFO("Initializing GPIO Controller");

    // Initialize all pins to default state
    reset();

    // Map memory-mapped I/O registers
    mapMemoryRegions();

    LOG_INFO("GPIO Controller initialized with {} pins", GPIO_PIN_COUNT);
    return true;
}

void GPIOController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& pin : pins_) {
        pin.mode = GPIOMode::INPUT;
        pin.level = GPIOLevel::LOW;
        pin.pull_up = false;
        pin.pull_down = false;
        pin.intr_type = GPIOInterruptType::NONE;
        pin.interrupt_pending = false;
        pin.pwm = {};
        pin.virtual_component = nullptr;
        pin.schmitt_output = false;
        pin.debounce_count = 0;
    }

    pwm_states_.clear();
    pwm_states_.resize(GPIO_PIN_COUNT);

    LOG_DEBUG("GPIO Controller reset");
}

int GPIOController::configurePin(uint8_t pin, GPIOMode mode,
                                   bool pull_up, bool pull_down,
                                   GPIOInterruptType intr_type) {
    if (pin >= GPIO_PIN_COUNT) {
        LOG_ERROR("Invalid GPIO pin: {}", pin);
        return ESP_ERR_INVALID_ARG;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if pin is input-only
    if (isInputOnly(pin) && mode != GPIOMode::INPUT &&
        mode != GPIOMode::INPUT_PULLUP &&
        mode != GPIOMode::INPUT_PULLDOWN) {
        LOG_WARN("GPIO{} is input-only, cannot set as output", pin);
        return ESP_ERR_INVALID_ARG;
    }

    auto& p = pins_[pin];
    p.mode = mode;
    p.pull_up = pull_up;
    p.pull_down = pull_down;
    p.intr_type = intr_type;

    LOG_DEBUG("GPIO{} configured: mode={}, pull_up={}, pull_down={}, intr_type={}",
              pin, static_cast<int>(mode), pull_up, pull_down, static_cast<int>(intr_type));

    return ESP_OK;
}

int GPIOController::setLevel(uint8_t pin, GPIOLevel level) {
    if (pin >= GPIO_PIN_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& pin_state = pins_[pin];

    // Only output modes can be driven
    if (pin_state.mode != GPIOMode::OUTPUT &&
        pin_state.mode != GPIOMode::OUTPUT_OPEN_DRAIN &&
        pin_state.mode != GPIOMode::OUTPUT_OD &&
        pin_state.mode != GPIOMode::PWM_OUTPUT) {
        LOG_WARN("Attempt to set level on non-output GPIO{}", pin);
        return ESP_ERR_INVALID_ARG;
    }

    pin_state.level = level;
    pin_state.last_voltage = level == GPIOLevel::HIGH ? 3.3f : 0.0f;

    // Notify connected virtual component
    if (pin_state.virtual_component) {
        // Emit signal (main thread will handle)
        Q_EMIT pinStateChanged(pin, level);
    }

    LOG_VERBOSE("GPIO{} set to {}", pin, level == GPIOLevel::HIGH ? "HIGH" : "LOW");
    return ESP_OK;
}

GPIOLevel GPIOController::getLevel(uint8_t pin) const {
    if (pin >= GPIO_PIN_COUNT) {
        return GPIOLevel::LOW;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return pins_[pin].level;
}

float GPIOController::getVoltage(uint8_t pin) const {
    if (pin >= GPIO_PIN_COUNT) {
        return 0.0f;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto& p = pins_[pin];

    switch (p.mode) {
        case GPIOMode::OUTPUT:
        case GPIOMode::OUTPUT_OPEN_DRAIN:
        case GPIOMode::OUTPUT_OD:
            return p.level == GPIOLevel::HIGH ? 3.3f : 0.0f;

        case GPIOMode::ADC_INPUT:
            return p.last_voltage;

        case GPIOMode::PWM_OUTPUT:
            return p.pwm.active ? (p.pwm.duty_cycle / 100.0f * 3.3f) : 0.0f;

        case GPIOMode::INPUT:
        case GPIOMode::INPUT_PULLUP:
        case GPIOMode::INPUT_PULLDOWN:
        default:
            // Return voltage from external source or pulled level
            return p.last_voltage;
    }
}

void GPIOController::setExternalVoltage(uint8_t pin, float voltage) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& p = pins_[pin];

    p.last_voltage = voltage;

    // Schmitt trigger simulation (for input pins)
    if (p.mode == GPIOMode::INPUT ||
        p.mode == GPIOMode::INPUT_PULLUP ||
        p.mode == GPIOMode::INPUT_PULLDOWN ||
        p.mode == GPIOMode::ADC_INPUT) {
        bool new_state = (schmittTrigger(pin, voltage) == GPIOLevel::HIGH);
        if (new_state != p.schmitt_output) {
            p.schmitt_output = new_state;
            p.level = new_state ? GPIOLevel::HIGH : GPIOLevel::LOW;

            // Check interrupt
            checkInterruptCondition(pin);
        }
    }
}

void GPIOController::connectComponent(uint8_t pin, void* component) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    pins_[pin].virtual_component = component;
    LOG_DEBUG("Component connected to GPIO{}", pin);
}

void GPIOController::disconnectComponent(uint8_t pin, void* component) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (pins_[pin].virtual_component == component) {
        pins_[pin].virtual_component = nullptr;
        LOG_DEBUG("Component disconnected from GPIO{}", pin);
    }
}

void GPIOController::configurePWM(uint8_t pin, uint32_t frequency_hz, uint32_t duty_cycle) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& p = pins_[pin];

    p.mode = GPIOMode::PWM_OUTPUT;
    p.pwm.frequency_hz = frequency_hz;
    p.pwm.duty_cycle = duty_cycle;
    p.pwm.active = true;
    p.pwm.channel = pin;  // Simplified: use GPIO pin as channel

    // Initialize PWM state
    PWMState& state = pwm_states_[pin];
    state.period_ticks = 80000000ULL / frequency_hz;  // Assuming 80MHz timer
    state.high_ticks = (state.period_ticks * duty_cycle) / 100;
    state.output_state = false;

    LOG_DEBUG("GPIO{} PWM configured: {} Hz, {}% duty", pin, frequency_hz, duty_cycle);
}

void GPIOController::setADCValue(uint8_t pin, uint16_t adc_value) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& p = pins_[pin];

    if (p.mode == GPIOMode::ADC_INPUT) {
        p.last_voltage = (adc_value / 4095.0f) * 3.3f;
    }
}

uint16_t GPIOController::getADCValue(uint8_t pin) const {
    if (pin >= GPIO_PIN_COUNT) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    const auto& p = pins_[pin];

    if (p.mode == GPIOMode::ADC_INPUT) {
        return static_cast<uint16_t>((p.last_voltage / 3.3f) * 4095.0f);
    }
    return 0;
}

void GPIOController::setDACValue(uint8_t pin, uint8_t value) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (pin == 25 || pin == 26) {
        auto& p = pins_[pin];
        p.last_voltage = (value / 255.0f) * 3.3f;
    }
}

bool GPIOController::checkInterrupt(uint8_t pin) const {
    if (pin >= GPIO_PIN_COUNT) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    return pins_[pin].interrupt_pending;
}

void GPIOController::clearInterrupt(uint8_t pin) {
    if (pin >= GPIO_PIN_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    pins_[pin].interrupt_pending = false;
}

void GPIOController::updatePWM(uint64_t elapsed_us) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (uint8_t pin = 0; pin < GPIO_PIN_COUNT; pin++) {
        auto& p = pins_[pin];
        if (p.mode != GPIOMode::PWM_OUTPUT || !p.pwm.active) continue;

        PWMState& state = pwm_states_[pin];
        state.counter += elapsed_us * 80;  // 80 MHz timer

        uint64_t period_ticks = state.period_ticks;
        if (period_ticks == 0) period_ticks = 1;

        if (state.counter >= period_ticks) {
            state.counter = 0;
            state.output_state = false;
        }

        if (state.counter < state.high_ticks) {
            state.output_state = true;
        }

        p.level = state.output_state ? GPIOLevel::HIGH : GPIOLevel::LOW;
    }
}

void GPIOController::simulate() {
    // Placeholder for more complex simulation
}

void GPIOController::mapMemoryRegions() {
    // Map GPIO registers
    // ESP32 GPIO is mapped at 0x3FF440000 (GPIO0) through 0x3FF44FFFC
    // This is simplified; actual ESP32 has complex peripheral mux

    // For now, let's just register an MMIO region and handle reads/writes
    // in readRegister/writeRegister methods
}

uint32_t GPIOController::readRegister(uint32_t address) {
    // Simplified - just return 0
    return 0;
}

void GPIOController::writeRegister(uint32_t address, uint32_t value) {
    // Simplified - handle GPIO output register writes
    // Extract GPIO number from address
    // This would need proper ESP32 GPIO peripheral mapping
}

const GPIOPinState* GPIOController::getPinState(uint8_t pin) const {
    if (pin >= GPIO_PIN_COUNT) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);
    return &pins_[pin];
}

void GPIOController::checkInterruptCondition(uint8_t pin) {
    auto& p = pins_[pin];
    if (p.intr_type == GPIOInterruptType::NONE) return;

    bool trigger = false;
    switch (p.intr_type) {
        case GPIOInterruptType::POS_EDGE:
            trigger = (p.level == GPIOLevel::HIGH && !p.schmitt_output);
            break;
        case GPIOInterruptType::NEG_EDGE:
            trigger = (p.level == GPIOLevel::LOW && p.schmitt_output);
            break;
        case GPIOInterruptType::ANY_EDGE:
            trigger = (p.level != (p.schmitt_output ? GPIOLevel::HIGH : GPIOLevel::LOW));
            break;
        default:
            break;
    }

    if (trigger) {
        p.interrupt_pending = true;
        Q_EMIT interruptRaised(pin, p.intr_type);
    }
}

GPIOLevel GPIOController::schmittTrigger(uint8_t /*pin*/, float voltage) {
    static constexpr float V_high = 1.4f;
    static constexpr float V_low = 1.0f;
    static std::map<uint8_t, bool> last_state;

    bool current_state = voltage > V_high;
    if (voltage < V_low) {
        current_state = false;
    }

    // Update last state
    bool prev = last_state[/*pin*/0];  // Simplified
    last_state[/*pin*/0] = current_state;

    return current_state ? GPIOLevel::HIGH : GPIOLevel::LOW;
}

} // namespace esp32sim
