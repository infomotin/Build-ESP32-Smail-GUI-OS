/**
 * @file gpio_controller.h
 * @brief GPIO peripheral simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

#include <QObject>
#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @enum GPIOMode
 * @brief GPIO operating modes
 */
enum class GPIOMode {
    INPUT = 0,
    OUTPUT = 1,
    OUTPUT_OPEN_DRAIN = 2,
    INPUT_PULLUP = 3,
    INPUT_PULLDOWN = 4,
    INPUT_PULLDOWN_ONLY = 5,
    OUTPUT_OD = 6,  // Open drain output
    ADC_INPUT = 7,
    DAC_OUTPUT = 8,
    PWM_OUTPUT = 9,
    GPIO_INPUT_OUTPUT = 10
};

/**
 * @enum GPIOLevel
 * @brief Logic levels
 */
enum class GPIOLevel {
    LOW = 0,
    HIGH = 1
};

/**
 * @enum GPIOInterruptType
 * @brief Interrupt trigger types
 */
enum class GPIOInterruptType {
    NONE = 0,
    POS_EDGE = 1,   // Rising edge
    NEG_EDGE = 2,   // Falling edge
    ANY_EDGE = 3,   // Both edges
    LOW_LEVEL = 4,  // Low level
    HIGH_LEVEL = 5  // High level
};

/**
 * @struct GPIOPinState
 * @brief Current state of a GPIO pin
 */
struct GPIOPinState {
    uint8_t pin_number;
    GPIOMode mode = GPIOMode::INPUT;
    GPIOLevel level = GPIOLevel::LOW;
    bool pull_up = false;
    bool pull_down = false;
    GPIOInterruptType intr_type = GPIOInterruptType::NONE;
    bool interrupt_pending = false;

    // Analog settings
    bool adc_enabled = false;
    bool dac_enabled = false;
    bool pwm_enabled = false;

    // PWM configuration
    struct {
        uint32_t frequency_hz = 1000;
        uint32_t duty_cycle = 0;  // 0-100%
        uint8_t channel = 0;
        bool active = false;
    } pwm;

    // Virtual component connection
    void* virtual_component = nullptr;

    // Digital filtering (Schmitt trigger)
    static constexpr float V_THRESH_HIGH = 1.4f;  // 1.4V high threshold (3.3V ref)
    static constexpr float V_THRESH_LOW = 1.0f;   // 1.0V low threshold
    static constexpr float V_SUPPLY = 3.3f;

    // Schmitt trigger simulation
    float last_voltage = 0.0f;
    bool schmitt_output = false;

    // Input filtering (debounce)
    uint32_t debounce_count = 0;
    static constexpr uint32_t DEBOUNCE_THRESHOLD = 5; // 5ms at 1kHz
};

/**
 * @class GPIOController
 * @brief Simulates ESP32 GPIO peripheral with all 34 pins
 *
 * This class handles:
 * - GPIO pin configuration (input, output, pull-ups, interrupts)
 * - Level changes and notification to virtual components
 * - PWM generation
 * - ADC/DAC simulation
 * - Schmitt trigger simulation for analog-like behavior
 */
class GPIOController {
public:
    GPIOController(MemoryModel* memory);
    ~GPIOController();

    /**
     * @brief Initialize the GPIO controller
     */
    bool initialize();

    /**
     * @brief Reset all pins to default state
     */
    void reset();

    /**
     * @brief Configure a GPIO pin
     * @param pin Pin number (0-39)
     * @param mode Pin mode
     * @param pull_up Enable pull-up resistor
     * @param pull_down Enable pull-down resistor
     * @param intr_type Interrupt type
     * @return ESP_OK on success, error code otherwise
     */
    int configurePin(uint8_t pin, GPIOMode mode,
                     bool pull_up = false, bool pull_down = false,
                     GPIOInterruptType intr_type = GPIOInterruptType::NONE);

    /**
     * @brief Set output level for a pin
     * @param pin Pin number
     * @param level Logic level
     * @return ESP_OK on success
     */
    int setLevel(uint8_t pin, GPIOLevel level);

    /**
     * @brief Get input level from a pin
     * @param pin Pin number
     */
    GPIOLevel getLevel(uint8_t pin) const;

    /**
     * @brief Get current voltage at pin (for ADC conversion)
     */
    float getVoltage(uint8_t pin) const;

    /**
     * @brief Set external voltage on a pin (from virtual component)
     * @param pin Pin number
     * @param voltage Voltage in volts (0-3.3V)
     */
    void setExternalVoltage(uint8_t pin, float voltage);

    /**
     * @brief Connect a virtual component to a pin
     */
    void connectComponent(uint8_t pin, void* component);

    /**
     * @brief Disconnect a virtual component from a pin
     */
    void disconnectComponent(uint8_t pin, void* component);

    /**
     * @brief Set PWM parameters
     */
    void configurePWM(uint8_t pin, uint32_t frequency_hz, uint32_t duty_cycle);

    /**
     * @brief Set ADC conversion result
     * @param pin Pin number
     * @param adc_value 12-bit ADC value (0-4095)
     */
    void setADCValue(uint8_t pin, uint16_t adc_value);

    /**
     * @brief Get ADC conversion for a pin
     */
    uint16_t getADCValue(uint8_t pin) const;

    /**
     * @brief Set DAC output
     * @param pin Pin number (25 or 26)
     * @param value 8-bit DAC value (0-255)
     */
    void setDACValue(uint8_t pin, uint8_t value);

    /**
     * @brief Check if interrupt is pending for a pin
     */
    bool checkInterrupt(uint8_t pin) const;

    /**
     * @brief Clear interrupt for a pin
     */
    void clearInterrupt(uint8_t pin);

    /**
     * @brief Get all pin states
     */
    const std::vector<GPIOPinState>& getAllPins() const { return pins_; }

    /**
     * @brief Get pin state
     */
    const GPIOPinState* getPinState(uint8_t pin) const;

    /**
     * @brief Update PWM outputs (call periodically)
     */
    void updatePWM(uint64_t elapsed_ns);

    /**
     * @brief Simulate GPIO behavior (Schmitt trigger, etc.)
     */
    void simulate();

    /**
     * @brief Map memory-mapped I/O registers
     */
    void mapMemoryRegions();

    /**
     * @brief Read from memory-mapped GPIO register
     */
    uint32_t readRegister(uint32_t address);

    /**
     * @brief Write to memory-mapped GPIO register
     */
    void writeRegister(uint32_t address, uint32_t value);

    /**
     * @brief Get number of GPIO pins
     */
    static constexpr uint8_t GPIO_PIN_COUNT = 40;

    /**
     * @brief Check if pin is input-only (34-39)
     */
    static bool isInputOnly(uint8_t pin) { return pin >= 34 && pin <= 39; }

    signals:
        /**
         * @brief Emitted when pin state changes
         */
        void pinStateChanged(uint8_t pin, GPIOLevel level);

        /**
         * @brief Emitted when interrupt occurs
         */
        void interruptRaised(uint8_t pin, GPIOInterruptType type);

private:
    MemoryModel* memory_ = nullptr;
    std::vector<GPIOPinState> pins_;

    // Mutex for thread safety
    mutable std::mutex mutex_;

    // PWM internal state
    struct PWMState {
        uint64_t counter = 0;
        bool output_state = false;
        uint32_t period_ticks = 0;
        uint32_t high_ticks = 0;
    };
    std::vector<PWMState> pwm_states_;

    // Internal methods
    void updatePinImpedance(uint8_t pin);
    void checkInterruptCondition(uint8_t pin);
    GPIOLevel schmittTrigger(uint8_t pin, float voltage);
    void notifyPins(uint8_t pin, GPIOLevel level);
    uint32_t extractField(uint32_t value, int lsb, int width);
    void insertField(uint32_t& reg, int lsb, int width, uint32_t value);
};

} // namespace esp32sim
