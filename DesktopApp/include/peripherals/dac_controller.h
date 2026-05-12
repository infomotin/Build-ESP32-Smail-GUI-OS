/**
 * @file dac_controller.h
 * @brief DAC peripheral simulation controller (stub)
 */

#ifndef PERIPHERALS_DAC_CONTROLLER_H
#define PERIPHERALS_DAC_CONTROLLER_H

#include <stdint.h>
#include <memory>
#include <mutex>

namespace esp32sim {

class MemoryModel;

/**
 * @class DACController
 * @brief Simulates ESP32 DAC (8-bit on GPIO25/26)
 *
 * This is a stub implementation for desktop simulation.
 */
class DACController {
public:
    explicit DACController(MemoryModel* memory);
    ~DACController();

    bool initialize();
    void reset();
    void mapMemoryRegions();
    void tick(uint64_t elapsed_ns);

    // Set DAC output value (0-255)
    void setOutput(uint8_t channel, uint8_t value);
    uint8_t getOutput(uint8_t channel) const;

private:
    MemoryModel* memory_ = nullptr;
    uint8_t output_[2] = {0, 0}; // Two DAC channels
    mutable std::mutex mutex_;
};

} // namespace esp32sim

#endif // PERIPHERALS_DAC_CONTROLLER_H
