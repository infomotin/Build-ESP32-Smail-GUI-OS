/**
 * @file spi_controller.cpp
 * @brief SPI controller stub implementation
 */

#include "peripherals/spi_controller.h"
#include "utils/logger.h"

namespace esp32sim {

SPIController::SPIController(uint8_t bus_id, MemoryModel* memory)
    : bus_id_(bus_id), memory_(memory) {
}

SPIController::~SPIController() = default;

bool SPIController::initialize() {
    LOG_DEBUG("SPIController {}: Initialized (stub)", bus_id_);
    return true;
}

void SPIController::reset() {
    LOG_DEBUG("SPIController {}: Reset", bus_id_);
}

void SPIController::setMode(SPIMode mode) {
    // stub
}

void SPIController::setClockSpeed(uint32_t speed_hz) {
    // stub
}

void SPIController::setBitOrder(/*...*/) {
    // stub
}

void SPIController::setDataMode(uint8_t mode) {
    // stub
}

void SPIController::setFrequency(uint32_t freq) {
    // stub
}

int SPIController::transfer(uint8_t* tx_data, uint8_t* rx_data, size_t len, uint32_t timeout_ms) {
    LOG_DEBUG("SPIController {}: transfer ({} bytes) - stub", bus_id_, len);
    return 0;
}

int SPIController::transferNB(uint8_t* tx_data, uint8_t* rx_data, size_t len) {
    // stub
    return 0;
}

void SPIController::addSlaveDevice(uint8_t cs_pin, const std::string& name) {
    // stub
}

void SPIController::removeSlaveDevice(uint8_t cs_pin) {
    // stub
}

void SPIController::tick(uint64_t elapsed_ns) {
    // stub
}

void SPIController::mapMemoryRegions() {
    LOG_DEBUG("SPIController {}: mapMemoryRegions (stub)", bus_id_);
}

uint32_t SPIController::readRegister(uint32_t address) {
    return 0;
}

void SPIController::writeRegister(uint32_t address, uint32_t value) {
    // stub
}

} // namespace esp32sim
