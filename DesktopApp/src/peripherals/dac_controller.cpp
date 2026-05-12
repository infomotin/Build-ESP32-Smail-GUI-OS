/**
 * @file dac_controller.cpp
 * @brief DAC controller stub implementation
 */

#include "peripherals/dac_controller.h"
#include "utils/logger.h"

namespace esp32sim {

DACController::DACController(MemoryModel* memory) : memory_(memory) {
}

DACController::~DACController() = default;

bool DACController::initialize() {
    LOG_DEBUG("DACController: Initialized (stub)");
    return true;
}

void DACController::reset() {
    LOG_DEBUG("DACController: Reset");
    output_[0] = output_[1] = 0;
}

void DACController::mapMemoryRegions() {
    // No MMIO mapping for stub
    LOG_DEBUG("DACController: mapMemoryRegions (no-op)");
}

void DACController::tick(uint64_t elapsed_ns) {
    // No simulation needed
}

void DACController::setOutput(uint8_t channel, uint8_t value) {
    if (channel < 2) {
        output_[channel] = value;
    }
}

uint8_t DACController::getOutput(uint8_t channel) const {
    return (channel < 2) ? output_[channel] : 0;
}

} // namespace esp32sim
