/**
 * @file adc_controller.cpp
 * @brief ADC controller implementation
 */

#include "peripherals/adc_controller.h"
#include "utils/logger.h"

namespace esp32sim {

ADCController::ADCController(MemoryModel* memory) : memory_(memory) {
    initializePinMapping();
    rng_.seed(std::random_device{}());
}

ADCController::~ADCController() = default;

bool ADCController::initialize() {
    LOG_INFO("ADC Controller initialized");
    return true;
}

void ADCController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& ch : channels_) {
        ch.enabled = false;
        ch.raw_value = 0;
        ch.voltage = 0.0f;
    }
}

void ADCController::configureChannel(uint8_t channel, ADCAttenuation attenuation, int8_t gpio_pin) {
    if (channel >= ADC_CHANNEL_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& ch = channels_[channel];
    ch.attenuation = attenuation;
    ch.gpio_pin = gpio_pin;

    // Set attenuation gain (simplified - actual ESP32 has specific values)
    switch (attenuation) {
        case ADCAttenuation::ATTEN_0DB:    ch.calibration_gain = 1.0f; break;
        case ADCAttenuation::ATTEN_2_5DB:  ch.calibration_gain = 1.25f; break;
        case ADCAttenuation::ATTEN_6DB:    ch.calibration_gain = 2.0f; break;
        case ADCAttenuation::ATTEN_11DB:   ch.calibration_gain = 3.5f; break;
    }

    LOG_DEBUG("ADC channel {} configured: attenuation={}, gpio={}",
              channel, static_cast<int>(attenuation), gpio_pin);
}

void ADCController::enableChannel(uint8_t channel, bool enable) {
    if (channel >= ADC_CHANNEL_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    channels_[channel].enabled = enable;
}

void ADCController::setAttenuation(uint8_t channel, ADCAttenuation attenuation) {
    configureChannel(channel, attenuation, channels_[channel].gpio_pin);
}

uint16_t ADCController::readRaw(uint8_t channel) {
    if (channel >= ADC_CHANNEL_COUNT) return 0;

    std::lock_guard<std::mutex> lock(mutex_);

    auto& ch = channels_[channel];
    if (!ch.enabled && ch.gpio_pin >= 0) {
        LOG_WARN("Reading from disabled ADC channel {}", channel);
    }

    // Get voltage
    float voltage = ch.voltage;

    // Convert to raw value
    uint16_t raw = static_cast<uint16_t>((voltage / 3.3f) * 4095.0f);

    // Add noise if enabled
    if (ch.noise_enabled) {
        raw = addNoise(raw, channel);
    }

    ch.raw_value = raw;
    ch.filtered_value = (ch.filtered_value * 3 + raw) / 4;  // Simple low-pass filter

    stats_.conversions++;
    stats_.total_voltage_read += voltage;

    return raw;
}

float ADCController::readVoltage(uint8_t channel) {
    uint16_t raw = readRaw(channel);
    return (raw / 4095.0f) * 3.3f;
}

uint16_t ADCController::getCurrentValue(uint8_t channel) const {
    if (channel >= ADC_CHANNEL_COUNT) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    return channels_[channel].raw_value;
}

void ADCController::setExternalVoltage(uint8_t pin, float voltage) {
    // Find channel for this pin
    uint8_t channel = pinToChannel(pin);
    if (channel != 0xFF) {
        std::lock_guard<std::mutex> lock(mutex_);
        channels_[channel].voltage = voltage;
    }
}

void ADCController::calibrateChannel(uint8_t channel, float offset, float gain) {
    if (channel >= ADC_CHANNEL_COUNT) return;

    std::lock_guard<std::mutex> lock(mutex_);
    channels_[channel].calibration_offset = offset;
    channels_[channel].calibration_gain = gain;
}

void ADCController::mapMemoryRegions() {
    // Map ADC registers (simplified)
    // Actual ESP32 ADC1 base: 0x3FF1E000, ADC2 base: 0x3FF1F000
    // We'll handle through readRegister/writeRegister
}

uint32_t ADCController::readRegister(uint32_t address) {
    // Simplified register read implementation
    return 0;
}

void ADCController::writeRegister(uint32_t address, uint32_t value) {
    // Simplified register write implementation
}

void ADCController::initializePinMapping() {
    // Fill pin_to_channel_ with -1 (no mapping)
    std::fill(pin_to_channel_.begin(), pin_to_channel_.end(), -1);

    // ADC1 channels mapping to GPIO pins (ESP32)
    // Channel 0 = GPIO36, 1 = GPIO37, 2 = GPIO38, 3 = GPIO39
    // Channel 4 = GPIO32, 5 = GPIO33, 6 = GPIO34, 7 = GPIO35
    // Channel 8 = GPIO0 (attenuation only)
    pin_to_channel_[36] = 0;
    pin_to_channel_[37] = 1;
    pin_to_channel_[38] = 2;
    pin_to_channel_[39] = 3;
    pin_to_channel_[32] = 4;
    pin_to_channel_[33] = 5;
    pin_to_channel_[34] = 6;
    pin_to_channel_[35] = 7;

    // ADC2 channels
    // Channel 0 = GPIO4, 1 = GPIO0, 2 = GPIO2, 3 = GPIO15
    // Channel 4 = GPIO13, 5 = GPIO12, 6 = GPIO14, 7 = GPIO27, 8 = GPIO25, 9 = GPIO26
    pin_to_channel_[4] = 9;   // ADC2_CH0 maps to channel offset
    pin_to_channel_[0] = 10;
    pin_to_channel_[2] = 11;
    pin_to_channel_[15] = 12;
    pin_to_channel_[13] = 13;
    pin_to_channel_[12] = 14;
    pin_to_channel_[14] = 15;
    pin_to_channel_[27] = 16;
    pin_to_channel_[25] = 17;
    pin_to_channel_[26] = 18;
}

uint8_t ADCController::pinToChannel(uint8_t pin) const {
    if (pin >= pin_to_channel_.size()) return 0xFF;
    int8_t ch = pin_to_channel_[pin];
    return (ch >= 0) ? static_cast<uint8_t>(ch) : 0xFF;
}

float ADCController::voltageToRaw(float voltage, uint8_t channel) const {
    // Simplified - ignores attenuation gain
    return voltage / 3.3f * 4095.0f;
}

float ADCController::rawToVoltage(uint16_t raw, uint8_t channel) const {
    return (raw / 4095.0f) * 3.3f;
}

uint16_t ADCController::addNoise(uint16_t raw, uint8_t channel) {
    std::normal_distribution<float> dist(0.0f, channels_[channel].noise_std_dev);
    float noisy = static_cast<float>(raw) + dist(rng_);
    return static_cast<uint16_t>(std::max(0.0f, std::min(4095.0f, noisy)));
}

} // namespace esp32sim
