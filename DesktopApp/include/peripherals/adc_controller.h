/**
 * @file adc_controller.h
 * @brief ADC (Analog-to-Digital Converter) simulation controller
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <random>
#include <mutex>
#include <array>

#include "simulator/core/memory/memory_model.h"

namespace esp32sim {

/**
 * @enum ADCAttenuation
 * @brief ADC attenuation levels
 */
enum class ADCAttenuation {
    ATTEN_0DB = 0,    // 0 dB attenuation, full-scale ~3.9V
    ATTEN_2_5DB = 1,  // 2.5 dB attenuation
    ATTEN_6DB = 2,    // 6 dB attenuation
    ATTEN_11DB = 3    // 11 dB attenuation
};

/**
 * @struct ADCChannel
 * @brief ADC channel configuration and state
 */
struct ADCChannel {
    uint8_t channel = 0;
    bool enabled = false;
    ADCAttenuation attenuation = ADCAttenuation::ATTEN_0DB;

    // Calibration
    float calibration_offset = 0.0f;
    float calibration_gain = 1.0f;

    // Current reading
    uint16_t raw_value = 0;     // 12-bit raw value
    float voltage = 0.0f;       // Actual voltage
    uint16_t filtered_value = 0;

    // Noise simulation
    bool noise_enabled = true;
    int32_t noise_seed = 42;
    float noise_std_dev = 2.0f;  // 2 LSB standard deviation

    // Connected pin
    int8_t gpio_pin = -1;  // -1 if not connected
};

/**
 * @class ADCController
 * @brief Simulates ESP32 ADC peripheral
 *
 * Features:
 * - 12-bit resolution
 * - 4 attenuation levels
 * - Noise injection (±2 LSB random variation)
 * - Support for both ADC1 and ADC2
 * - Calibration support
 */
class ADCController {
public:
    explicit ADCController(MemoryModel* memory);
    ~ADCController();

    /**
     * @brief Initialize ADC controller
     */
    bool initialize();

    /**
     * @brief Reset all channels
     */
    void reset();

    /**
     * @brief Configure ADC channel
     */
    void configureChannel(uint8_t channel, ADCAttenuation attenuation,
                          int8_t gpio_pin = -1);

    /**
     * @brief Enable a channel
     */
    void enableChannel(uint8_t channel, bool enable = true);

    /**
     * @brief Set attenuation for a channel
     */
    void setAttenuation(uint8_t channel, ADCAttenuation attenuation);

    /**
     * @brief Read raw ADC value (trigger conversion)
     * @param channel Channel number (0-15 for ADC1, 0-9 for ADC2)
     * @return 12-bit ADC value (0-4095)
     */
    uint16_t readRaw(uint8_t channel);

    /**
     * @brief Read voltage (synchronous conversion)
     */
    float readVoltage(uint8_t channel);

    /**
     * @brief Get current raw value without triggering conversion
     */
    uint16_t getCurrentValue(uint8_t channel) const;

    /**
     * @brief Set external voltage on a pin (from virtual component)
     */
    void setExternalVoltage(uint8_t pin, float voltage);

    /**
     * @brief Set calibration parameters
     */
    void calibrateChannel(uint8_t channel, float offset, float gain);

    /**
     * @brief Enable/disable noise injection
     */
    void setNoiseEnabled(bool enabled) { noise_enabled_ = enabled; }

    /**
     * @brief Set noise standard deviation in LSB
     */
    void setNoiseLevel(float std_dev_lsb) { noise_std_dev_ = std_dev_lsb; }

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
     * @brief Get ADC statistics
     */
    struct Stats {
        uint64_t conversions = 0;
        uint64_t average_conversion_time_us = 0;
        float total_voltage_read = 0.0f;
    };
    const Stats& stats() const { return stats_; }

    /**
     * @brief Get number of channels
     */
    static constexpr uint8_t ADC_CHANNEL_COUNT = 18;  // 9 for ADC1, 9 for ADC2

private:
    MemoryModel* memory_ = nullptr;
    std::array<ADCChannel, ADC_CHANNEL_COUNT> channels_;

    // Pin to channel mapping
    std::array<int8_t, 40> pin_to_channel_;  // Maps GPIO pin to ADC channel, -1 if not mappable

    // Noise generation
    std::mt19937 rng_;
    bool noise_enabled_ = true;
    float noise_std_dev_ = 2.0f;

    // Timing
    uint32_t conversion_time_us_ = 1;    // 1 microsecond typical
    uint64_t last_conversion_end_ = 0;

    // Statistics
    Stats stats_;

    // Mutex
    std::mutex mutex_;

    // Internal methods
    void initializePinMapping();
    uint8_t pinToChannel(uint8_t pin) const;
    float voltageToRaw(float voltage, uint8_t channel) const;
    float rawToVoltage(uint16_t raw, uint8_t channel) const;
    uint16_t addNoise(uint16_t raw, uint8_t channel);
    void updateConversions(uint64_t current_time);
};

} // namespace esp32sim
