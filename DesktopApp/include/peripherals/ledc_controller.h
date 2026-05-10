/**
 * @file ledc_controller.h
 * @brief LEDC (PWM) peripheral simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <array>
#include <memory>

#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @struct LEDCChannel
 * @brief Configuration for a single LEDC channel
 */
struct LEDCChannel {
    uint8_t channel = 0;           // Channel number (0-15)
    uint8_t gpio_pin = 0;         // Associated GPIO pin
    uint32_t frequency_hz = 5000; // PWM frequency (1 Hz - 40 MHz)
    uint32_t duty_cycle = 0;      // Duty cycle (0-100%)
    uint32_t resolution_bits = 10; // Resolution (1-20 bits)
    bool enabled = false;
    bool inverted = false;        // Inverted output

    // Internal state
    uint64_t timer_counter = 0;
    bool output_level = false;
    uint64_t period_ticks = 0;
    uint64_t high_ticks = 0;
};

/**
 * @class LEDCController
 * @brief Simulates ESP32 LEDC (PWM) controller
 *
 * Features:
 * - 16 independent PWM channels
 * - Configurable frequency (1 Hz - 40 MHz)
 * - Resolution from 1 to 20 bits
 * - Phase and duty control
 * - Supports LED dimming and motor control
 */
class LEDCController {
public:
    explicit LEDCController(MemoryModel* memory);
    ~LEDCController();

    /**
     * @brief Initialize LEDC
     */
    bool initialize();

    /**
     * @brief Reset all channels
     */
    void reset();

    /**
     * @brief Configure a PWM channel
     */
    int configureChannel(uint8_t channel, uint8_t gpio_pin,
                        uint32_t frequency_hz, uint32_t duty_cycle,
                        uint8_t resolution_bits = 10, bool invert = false);

    /**
     * @brief Set duty cycle
     */
    int setDutyCycle(uint8_t channel, uint32_t duty_cycle);

    /**
     * @brief Get duty cycle
     */
    uint32_t dutyCycle(uint8_t channel) const;

    /**
     * @brief Set frequency
     */
    int setFrequency(uint8_t channel, uint32_t frequency_hz);

    /**
     * @brief Get frequency
     */
    uint32_t frequency(uint8_t channel) const;

    /**
     * @brief Enable/disable channel
     */
    int setChannelEnable(uint8_t channel, bool enable);

    /**
     * @brief Check if channel is enabled
     */
    bool isChannelEnabled(uint8_t channel) const;

    /**
     * @brief Get output level
     */
    bool getOutputLevel(uint8_t channel) const;

    /**
     * @brief Map memory-mapped registers
     */
    void mapMemoryRegions();

    /**
     * @brief Read register
     */
    uint32_t readRegister(uint32_t address);

    /**
     * @brief Write register
     */
    void writeRegister(uint32_t address, uint32_t value);

    /**
     * @brief Update PWM outputs (call periodically)
     */
    void update(uint64_t elapsed_ns);

    /**
     * @brief Get channel info
     */
    const LEDCChannel* getChannel(uint8_t channel) const;

    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t channel_activations = 0;
        uint64_t duty_changes = 0;
        uint64_t frequency_changes = 0;
    };
    const Stats& stats() const { return stats_; }

private:
    MemoryModel* memory_ = nullptr;
    std::array<LEDCChannel, 16> channels_;
    std::mutex mutex_;

    // High-speed timer reference
    uint64_t global_timer_ = 0;
    static constexpr uint64_t TIMER_CLOCK_HZ = 80 * 1000 * 1000; // 80 MHz

    // Statistics
    Stats stats_;

    // Internal methods
    void calculatePWM(LEDCChannel& ch);
    uint64_t computePeriodTicks(uint32_t frequency_hz, uint8_t resolution_bits) const;
    uint64_t computeHighTicks(uint32_t duty_cycle, uint64_t period_ticks) const;
    void updateChannelOutput(uint8_t channel);
};

} // namespace esp32sim
