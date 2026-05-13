/**
 * @file i2c_controller.cpp
 * @brief I2C controller stub implementation
 */

#include "peripherals/i2c_controller.h"
#include "utils/logger.h"

namespace esp32sim {

I2CController::I2CController(uint8_t bus_id, MemoryModel* memory, QObject* parent)
    : QObject(parent), bus_id_(bus_id), memory_(memory) {
}

I2CController::~I2CController() = default;

bool I2CController::initialize() {
    LOG_DEBUG("I2CController {}: Initialized (stub)", bus_id_);
    return true;
}

void I2CController::reset() {
    LOG_DEBUG("I2CController {}: Reset", bus_id_);
}

void I2CController::setMode(I2CMode mode) {
    // stub
}

void I2CController::setClockSpeed(I2CClockSpeed speed) {
    // stub
}

void I2CController::start() {
    // stub
}

void I2CController::stop() {
    // stub
}

int I2CController::masterWrite(uint8_t slave_addr, const std::vector<uint8_t>& data, uint32_t timeout_ms) {
    LOG_DEBUG("I2CController {}: masterWrite to 0x{:02X} ({} bytes) - stub", bus_id_, slave_addr, data.size());
    return 0; // ESP_OK
}

int I2CController::masterRead(uint8_t slave_addr, std::vector<uint8_t>& data, uint32_t timeout_ms) {
    LOG_DEBUG("I2CController {}: masterRead from 0x{:02X} - stub", bus_id_, slave_addr);
    data.clear();
    return 0;
}

int I2CController::masterWriteRead(uint8_t slave_addr,
                                   const std::vector<uint8_t>& write_data,
                                   std::vector<uint8_t>& read_data,
                                   uint32_t timeout_ms) {
    LOG_DEBUG("I2CController {}: masterWriteRead to 0x{:02X} - stub", bus_id_, slave_addr);
    read_data.clear();
    return 0;
}

void I2CController::registerSlaveDevice(uint8_t address, std::shared_ptr<I2CSlaveDevice> device) {
    // stub
}

void I2CController::unregisterSlaveDevice(uint8_t address) {
    // stub
}

void I2CController::tick(uint64_t elapsed_ns) {
    // stub timing
}

void I2CController::mapMemoryRegions() {
    LOG_DEBUG("I2CController {}: mapMemoryRegions (stub)", bus_id_);
}

uint32_t I2CController::readRegister(uint32_t address) {
    return 0;
}

void I2CController::writeRegister(uint32_t address, uint32_t value) {
    // stub
}

} // namespace esp32sim
